//
// url + host: parsing, string reconstruction, and Public-Suffix-List
// matching - everything liburlparser's core does, in one file. (Previously
// split across url.h + url.cpp + urlparser_url.cpp + psl.h + psl.cpp +
// urlparser_host.cpp, with a confusing detail::url class wrapped by another
// url::Impl, and a PIMPL host::Impl wrapping a separately-loaded PSL.) url,
// hostname, and psl are now flat value types declared directly in
// include/urlparser.h - no PIMPL, no detail namespace, for any of that
// old structure. The one exception is psl::levels_ itself: it's a
// std::unique_ptr<detail::suffix_table> (defined below) specifically so
// urlparser.h doesn't have to include the vendored
// src/ankerl/unordered_dense.h just to declare a private member -
// see that class's definition below, and src/ankerl/README.md.
//
#include "urlparser.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>

// ---------------------------------------------------------------------
// SIMD scanning for find_first_of_3() below. Three tiers, each guarded
// by what it actually needs to be safe to use:
//
//   AVX2   x86-64, NOT guaranteed present on every CPU (unlike the two
//          below) - real chips still ship without it - so it needs an
//          actual runtime check, done once and cached, before it's
//          safe to call. That check itself, and how one function gets
//          AVX2 code generation without forcing -mavx2 (and therefore
//          an AVX2 requirement) on the rest of the binary, is done
//          differently per compiler - see cpu_has_avx2() below.
//   SSE2   x86-64's baseline ABI - literally every x86-64 CPU has it,
//          no detection needed, just an architecture #if.
//   NEON   aarch64's baseline ABI - same story as SSE2, just on ARM.
//
// Anything else (32-bit ARM without NEON, RISC-V, ...) falls back to
// the plain scalar loop at the bottom of find_first_of_3().
// ---------------------------------------------------------------------
#if defined(__x86_64__) || defined(_M_X64)
#define URLPARSER_HAS_SSE2 1
#include <emmintrin.h>
#if defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)
// GCC/Clang: __attribute__((target("avx2"))) below opts just that one
// function into AVX2 code generation - MSVC has no equivalent
// attribute, but MSVC also doesn't need one: it always compiles
// <immintrin.h> AVX2 intrinsics available regardless of /arch:, it's
// only GCC/Clang that refuse to without either -mavx2 or the target
// attribute. So MSVC gets the exact same function, just compiled
// straightforwardly.
#define URLPARSER_HAS_AVX2_DISPATCH 1
#include <immintrin.h>
#endif
#if defined(_MSC_VER)
#include <intrin.h>  // __cpuid/__cpuidex/_xgetbv - MSVC's equivalent of
                      // __builtin_cpu_supports(), see cpu_has_avx2().
#endif
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON) || \
    defined(__ARM_NEON__)
#define URLPARSER_HAS_NEON 1
#include <arm_neon.h>
#endif

#include "ankerl/unordered_dense.h"
#include "public_suffix_list_dat.h"

namespace {

// ::tolower goes through the current C locale on every call; URLs are
// ASCII-only per RFC 3986 (IDNs arrive already punycode-encoded), so a
// branchless ASCII-only version is both correct and several times faster.
inline char ascii_tolower(char c) noexcept {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
}

#if defined(URLPARSER_HAS_SSE2) || defined(URLPARSER_HAS_AVX2_DISPATCH)
// __builtin_ctz (count trailing zero bits, used to turn a SIMD
// compare-mask into "which lane matched first") is a GCC/Clang builtin -
// MSVC's equivalent is the _BitScanForward intrinsic. One place to
// branch on compiler instead of repeating the #if at every call site.
inline unsigned count_trailing_zeros(unsigned mask) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    return static_cast<unsigned>(__builtin_ctz(mask));
#elif defined(_MSC_VER)
    unsigned long index;
    _BitScanForward(&index, mask);
    return index;
#else
    unsigned index = 0;
    while ((mask & 1u) == 0) {
        mask >>= 1;
        ++index;
    }
    return index;
#endif
}
#endif

#if defined(URLPARSER_HAS_AVX2_DISPATCH)
// Runtime AVX2 check, done once (function-local static, thread-safe
// initialization per C++11) and cached - every call after the first is
// just a bool read.
//
// GCC/Clang's __builtin_cpu_supports("avx2") already does this
// correctly on its own: it doesn't just check that the CPU silicon has
// AVX2, it also confirms (internally, via the same XGETBV mechanism
// spelled out by hand below) that the operating system has enabled
// AVX/YMM register state saving - a CPU can support AVX2 while an old
// OS still leaves it disabled, and using AVX2 registers in that case
// takes down the process. MSVC has no equivalent builtin, so the OSXSAVE
// + XGETBV + CPUID-leaf-7 check is spelled out here by hand to get the
// exact same guarantee.
inline bool cpu_has_avx2() noexcept {
    static const bool has = [] {
#if defined(__GNUC__) || defined(__clang__)
        return static_cast<bool>(__builtin_cpu_supports("avx2"));
#elif defined(_MSC_VER)
        int regs1[4] = {0, 0, 0, 0};
        __cpuid(regs1, 1);
        const bool osxsave = (regs1[2] & (1 << 27)) != 0;  // ECX bit 27
        if (!osxsave) return false;
        const unsigned long long xcr0 = _xgetbv(0);
        const bool os_saves_ymm = (xcr0 & 0x6) == 0x6;  // XMM (bit1) + YMM (bit2)
        if (!os_saves_ymm) return false;
        int regs7[4] = {0, 0, 0, 0};
        __cpuidex(regs7, 7, 0);
        return (regs7[1] & (1 << 5)) != 0;  // EBX bit 5 = AVX2
#else
        return false;
#endif
    }();
    return has;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("avx2")))
#endif
inline bool
find_first_of_3_avx2(const char* data, size_t& i, size_t len, char c0, char c1,
                      char c2) noexcept {
    const __m256i v0 = _mm256_set1_epi8(c0);
    const __m256i v1 = _mm256_set1_epi8(c1);
    const __m256i v2 = _mm256_set1_epi8(c2);
    for (; i + 32 <= len; i += 32) {
        const __m256i chunk =
            _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));
        __m256i eq = _mm256_cmpeq_epi8(chunk, v0);
        eq = _mm256_or_si256(eq, _mm256_cmpeq_epi8(chunk, v1));
        eq = _mm256_or_si256(eq, _mm256_cmpeq_epi8(chunk, v2));
        const unsigned mask = static_cast<unsigned>(_mm256_movemask_epi8(eq));
        if (mask != 0) {
            i += count_trailing_zeros(mask);
            return true;
        }
    }
    return false;
}
#endif  // URLPARSER_HAS_AVX2_DISPATCH

// Finds the first occurrence of any of up to 3 target chars in
// [start, len). Pass the same char twice (or three times) to search for
// fewer than 3 - duplicates cost nothing extra since they fold into the
// same compare-and-OR chain. Returns `len` if none of them appear.
//
// Tries AVX2 (32 bytes/iteration) first when the running CPU actually
// has it, then SSE2/NEON (16 bytes/iteration, always available on their
// respective architectures) for whatever's left, then a plain
// byte-at-a-time loop for the final under-one-vector tail.
//
// This is the same "vectorized scan for a small set of delimiter bytes"
// idea ada-url uses for path/query/fragment scanning (see ada's
// scan_path_run() and friends in src/parser.cpp) - just the simple
// compare-and-mask version rather than their pshufb/tbl nibble-table
// one. That fancier technique earns its keep when classifying dozens of
// characters against multiple classes at once; here it's always exactly
// 3 (or fewer) fixed bytes, so a plain SIMD compare is simpler to get
// right and just as fast for this.
inline size_t find_first_of_3(const char* data, size_t start, size_t len, char c0, char c1,
                               char c2) noexcept {
    size_t i = start;
#if defined(URLPARSER_HAS_AVX2_DISPATCH)
    if (cpu_has_avx2() && find_first_of_3_avx2(data, i, len, c0, c1, c2)) {
        return i;
    }
#endif
#if defined(URLPARSER_HAS_SSE2)
    const __m128i v0 = _mm_set1_epi8(c0);
    const __m128i v1 = _mm_set1_epi8(c1);
    const __m128i v2 = _mm_set1_epi8(c2);
    for (; i + 16 <= len; i += 16) {
        const __m128i chunk =
            _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + i));
        __m128i eq = _mm_cmpeq_epi8(chunk, v0);
        eq = _mm_or_si128(eq, _mm_cmpeq_epi8(chunk, v1));
        eq = _mm_or_si128(eq, _mm_cmpeq_epi8(chunk, v2));
        const unsigned mask = static_cast<unsigned>(_mm_movemask_epi8(eq));
        if (mask != 0) {
            return i + count_trailing_zeros(mask);
        }
    }
#elif defined(URLPARSER_HAS_NEON)
    const uint8x16_t v0 = vdupq_n_u8(static_cast<uint8_t>(c0));
    const uint8x16_t v1 = vdupq_n_u8(static_cast<uint8_t>(c1));
    const uint8x16_t v2 = vdupq_n_u8(static_cast<uint8_t>(c2));
    for (; i + 16 <= len; i += 16) {
        const uint8x16_t chunk =
            vld1q_u8(reinterpret_cast<const uint8_t*>(data + i));
        uint8x16_t eq = vceqq_u8(chunk, v0);
        eq = vorrq_u8(eq, vceqq_u8(chunk, v1));
        eq = vorrq_u8(eq, vceqq_u8(chunk, v2));
        // Narrow each lane's 0xFF/0x00 to one bit via a "pair max"
        // reduction so we can test "was anything set" in one go, then
        // fall back to a byte-by-byte scan of just this 16-byte chunk
        // to find exactly which lane - NEON has no direct movemask
        // equivalent, but this window is small (16 bytes) either way.
        if (vmaxvq_u8(eq) != 0) {
            for (size_t j = 0; j < 16; ++j) {
                char c = data[i + j];
                if (c == c0 || c == c1 || c == c2) return i + j;
            }
        }
    }
#endif
    for (; i < len; ++i) {
        char c = data[i];
        if (c == c0 || c == c1 || c == c2) return i;
    }
    return len;
}

// scheme := ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )  (RFC 3986 §3.1)
constexpr bool is_scheme_char(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
}
constexpr bool is_digit_char(char c) noexcept { return c >= '0' && c <= '9'; }

struct AuthorityBounds {
    size_t host_start;
    size_t host_end;
    size_t at_pos;        // std::string::npos if there's no userinfo
    size_t colon_pos;     // std::string::npos if there's no port
    size_t authority_end;
};

// Single forward pass over the authority section starting at `position`
// (right after "scheme://"): finds where it ends (first '/', '?' or '#')
// and, within that range, the userinfo '@' separator and the port ':'
// separator. This is the ONE place that decides where a host starts and
// ends within a URL - both url::url() (the full parser) and
// url::extract_host() (the cheap host-only shortcut) call this, so they can
// never disagree with each other about port/userinfo handling the way two
// independently-written scans eventually would.
//
// An IPv6 host is written in brackets ("[::1]:8080") specifically so the
// address's own colons don't get confused with the port separator - so
// colons between '[' and ']' are skipped when looking for colon_pos.
AuthorityBounds scan_authority(std::string_view url, size_t position) noexcept {
    size_t authority_end = url.length();
    size_t at_pos = std::string::npos;
    size_t colon_pos = std::string::npos;
    bool in_ip_literal = false;
    for (size_t i = position; i < url.length(); ++i) {
        char c = url[i];
        if (c == '/' || c == '?' || c == '#') {
            authority_end = i;
            break;
        }
        if (c == '[') {
            in_ip_literal = true;
        } else if (c == ']') {
            in_ip_literal = false;
        } else if (c == '@') {
            at_pos = i;
            colon_pos = std::string::npos;  // a ':' before '@' is userinfo, not a port
        } else if (c == ':' && !in_ip_literal && colon_pos == std::string::npos) {
            colon_pos = i;
        }
    }
    const size_t host_start = (at_pos != std::string::npos) ? at_pos + 1 : position;
    const size_t host_end = (colon_pos != std::string::npos) ? colon_pos : authority_end;
    return {host_start, host_end, at_pos, colon_pos, authority_end};
}

// Membership tests below (is-known-protocol, uses-netloc, uses-params) run
// on every url::url() parse. Measured before settling on this: a custom
// flat-array set (avoiding unordered_set<string>'s per-lookup temporary
// std::string) was tried here first, on the theory that a node-based
// container would be cache-unfriendly for this. That theory didn't survive
// contact with a real measurement - std::set<string, std::less<>> came out
// at least as fast in a 2M-lookup micro-benchmark against the flat-array
// version, so the extra machinery bought nothing. std::less<> (a
// *transparent* comparator) is what actually matters: it lets find() take
// the parsed scheme by std::string_view directly, so no temporary
// std::string is constructed just to get a comparable key.
const std::set<std::string, std::less<>> USES_NETLOC = {
    "",     "file",  "ftp",   "git",   "git+ssh", "gopher", "http",
    "https", "imap", "mms",   "nfs",   "nntp",    "prospero", "rsync",
    "rtsp", "rtspu", "sftp",  "shttp", "snews",   "svn",    "svn+ssh",
    "telnet", "wais"};

const std::set<std::string, std::less<>> USES_PARAMS = {
    "",   "ftp",  "hdl",   "http", "https", "imap", "mms",
    "prospero", "rtsp", "rtspu", "sftp", "shttp", "sip", "sips", "tel"};

const std::set<std::string, std::less<>> KNOWN_PROTOCOLS = {
    "",    "file",  "ftp",     "git",  "git+ssh", "gopher", "hdl",
    "http", "https", "imap",   "mms",  "nfs",     "nntp",   "prospero",
    "rsync", "rtsp", "rtspu",  "sftp", "shttp",   "sip",    "sips",
    "sms", "snews", "svn",    "svn+ssh", "tel",   "telnet", "wais"};

std::vector<std::string> split(const std::string& str, const char delim) noexcept {
    std::vector<std::string> strings;
    size_t start;
    size_t end = 0;
    while ((start = str.find_first_not_of(delim, end)) != std::string::npos) {
        end = str.find(delim, start);
        strings.push_back(str.substr(start, end - start));
    }
    return strings;
}

}  // namespace

bool urlparser::url::is_psl_loaded() noexcept { return urlparser::psl::instance().is_loaded(); }

urlparser::url::url(std::string_view url, const bool ignore_www)
    : ignore_www_(ignore_www) {
    // Every field is a non-expanding substring of url (case-folding only
    // changes case, never length), so their combined length can never
    // exceed url.size() - reserving exactly that means storage_ never
    // reallocates during parsing, so the Spans we hand out below stay valid
    // forever.
    storage_.reserve(url.size());

    auto appendAsIs = [this](std::string_view src) -> Span {
        const auto pos = static_cast<uint32_t>(storage_.size());
        storage_.append(src);
        return Span{pos, static_cast<uint32_t>(src.size())};
    };
    auto appendLower = [&appendAsIs, this](std::string_view src) -> Span {
        const Span s = appendAsIs(src);
        std::transform(storage_.begin() + s.pos, storage_.begin() + s.pos + s.len,
                        storage_.begin() + s.pos, ascii_tolower);
        return s;
    };

    size_t position = 0;
    size_t index = url.find(':');
    if (index != std::string::npos) {
        // All the characters in our would-be scheme must be scheme chars.
        if (std::all_of(url.begin(), url.begin() + index, is_scheme_char)) {
            // If there's nothing after the ':', or any non-digit follows,
            // this is a scheme (not e.g. "host:1234").
            if ((index + 1) >= url.length() ||
                std::any_of(url.begin() + index + 1, url.end(),
                            [](char c) { return !is_digit_char(c); })) {
                scheme_ = appendLower(url.substr(0, index));
                position = index + 1;
            } else {
                scheme_ = appendLower(url.substr(0, index));
                if (KNOWN_PROTOCOLS.find(field(scheme_)) != KNOWN_PROTOCOLS.end()) {
                    position = index + 1;
                } else {
                    scheme_ = Span{};
                }
            }
        }
    }

    // Search for the netloc.
    if ((url.length() - position) >= 1 && url[position] == '/' &&
        url[position + 1] == '/') {
        // Skip the '//'.
        position += 2;

        // Single forward pass over the authority section - see scan_authority().
        const auto authority = scan_authority(url, position);
        const size_t host_start = authority.host_start;
        const size_t host_end = authority.host_end;
        const size_t at_pos = authority.at_pos;
        const size_t colon_pos = authority.colon_pos;
        const size_t authority_end = authority.authority_end;

        if (at_pos != std::string::npos) {
            userinfo_ = appendAsIs(url.substr(position, at_pos - position));
        }

        host_ = appendLower(url.substr(host_start, host_end - host_start));
        if (ignore_www_) {
            // Cheap (a prefix check, not a PSL lookup) - do it once, here,
            // rather than lazily: host_ is then a plain, immutable-after-
            // construction field like everything else, and str()/
            // full_domain()/host() all agree with each other from the start.
            // With Span-based storage this is just an offset bump - no
            // copy, no allocation at all.
            if (field(host_).compare(0, 4, "www.") == 0) {
                host_.pos += 4;
                host_.len -= 4;
            }
        }

        position = authority_end;

        // Extract the port, if any. std::from_chars avoids both an
        // allocation (no intermediate substring) and stoi's locale-aware,
        // exception-on-success-path machinery.
        if (colon_pos != std::string::npos) {
            const char* first = url.data() + colon_pos + 1;
            const char* last = url.data() + authority_end;
            if (first != last) {
                int parsed_port = 0;
                auto [ptr, ec] = std::from_chars(first, last, parsed_port);
                if (ec != std::errc() || ptr != last) {
                    throw std::invalid_argument("Port not a number: " + std::string(first, last));
                }
                if (parsed_port > 65535 || parsed_port < 0) {
                    throw std::invalid_argument("Port out of range: " + std::string(first, last));
                }
                port_ = parsed_port;
            }
        }
    }

    // Locate the first '#' (fragment), the first '?' before it (query),
    // and - only for schemes that use params - the first ';' before the
    // query. Same semantics as a single byte-by-byte forward pass would
    // give: whichever of '#'/'?'/';' the scan reaches first determines
    // what happens next, and each match narrows what we search for from
    // there (a '?' means only '#' can still matter; ';' only counts
    // before either '#' or '?' has been seen). We still do that in
    // forward-only passes over shrinking suffixes of the string, just
    // with each pass vectorized via find_first_of_3() instead of one
    // std::string::npos-comparison per byte.
    const bool track_params = USES_PARAMS.find(field(scheme_)) != USES_PARAMS.end();
    size_t hash_pos = std::string::npos;
    size_t query_pos = std::string::npos;
    size_t params_pos = std::string::npos;

    {
        size_t i = position;
        bool seen_query = false;
        bool seen_params = false;
        while (i < url.length()) {
            const bool want_semicolon = track_params && !seen_query && !seen_params;
            const size_t found = find_first_of_3(
                url.data(), i, url.length(), '#', seen_query ? '#' : '?',
                want_semicolon ? ';' : '#');
            if (found == url.length()) break;

            const char c = url[found];
            if (c == '#') {
                hash_pos = found;
                break;
            }
            if (c == '?' && !seen_query) {
                query_pos = found;
                seen_query = true;
            } else if (c == ';' && want_semicolon) {
                params_pos = found;
                seen_params = true;
            }
            i = found + 1;
        }
    }


    const size_t path_end = (query_pos != std::string::npos)  ? query_pos
                            : (hash_pos != std::string::npos) ? hash_pos
                                                                : url.length();
    const size_t path_content_end = (params_pos != std::string::npos) ? params_pos : path_end;

    path_ = appendAsIs(url.substr(position, path_content_end - position));

    if (params_pos != std::string::npos) {
        params_ = appendAsIs(url.substr(params_pos + 1, path_end - params_pos - 1));
        has_params_ = true;
    }

    if (query_pos != std::string::npos) {
        const size_t query_end = (hash_pos != std::string::npos) ? hash_pos : url.length();
        query_ = appendAsIs(url.substr(query_pos + 1, query_end - query_pos - 1));
        has_query_ = true;
    }

    if (hash_pos != std::string::npos) {
        fragment_ = appendAsIs(url.substr(hash_pos + 1, url.length() - hash_pos - 1));
    }
}

std::string urlparser::url::str() const noexcept {
    std::string result;
    result.reserve(storage_.size() + 16);

    const std::string_view scheme = field(scheme_);
    const std::string_view userinfo = field(userinfo_);
    const std::string_view host = field(host_);
    const std::string_view path = field(path_);
    const std::string_view params = field(params_);
    const std::string_view query = field(query_);
    const std::string_view fragment = field(fragment_);

    if (!scheme.empty()) {
        result.append(scheme);
        result.append(USES_NETLOC.find(scheme) != USES_NETLOC.end() ? "://" : ":");
    } else if (!host.empty()) {
        result.append("//");
    }

    if (!userinfo.empty()) {
        result.append(userinfo);
        result.append("@");
    }

    if (!host.empty()) {
        result.append(host);
    }

    if (port_) {
        result.append(":");
        result.append(std::to_string(port_));
    }

    if (path.empty()) {
        if (!result.empty()) {
            result.append("/");
        }
    } else {
        if (!host.empty() && path[0] != '/') {
            result.append(1, '/');
        }
        result.append(path);
    }

    if (has_params_) {
        result.append(";");
        result.append(params);
    }

    if (has_query_) {
        result.append("?");
        result.append(query);
    }

    if (!fragment.empty()) {
        result.append("#");
        result.append(fragment);
    }

    return result;
}

std::string urlparser::url::abspath() const noexcept {
    // Resolves '.'/'..' path segments, the way a filesystem path resolver
    // would - a pure computation (no mutation of any internal state).
    const std::string_view path = field(path_);
    std::string result;
    std::vector<size_t> segment_starts;

    if (!path.empty() && path[0] == '/') {
        result.append(1, '/');
        segment_starts.push_back(0);
    }

    size_t previous = 0;
    size_t index = path.find('/');
    auto emit_segment = [&](size_t start, size_t end) {
        if (end - start == 0) return;  // skip empty segments
        const std::string_view segment = path.substr(start, end - start);
        if (segment == ".") return;
        if (segment == "..") {
            if (segment_starts.size() > 1) {
                result.resize(segment_starts.back());
                segment_starts.pop_back();
            }
            return;
        }
        segment_starts.push_back(result.size());
        if (!result.empty() && result.back() != '/') result.append(1, '/');
        result.append(segment);
    };

    for (; index != std::string::npos; previous = index + 1, index = path.find('/', index + 1)) {
        emit_segment(previous, index);
    }
    emit_segment(previous, path.size());

    return result;
}

urlparser::QueryParams urlparser::url::params() const noexcept {
    return split(std::string(field(query_)), '&');
}

namespace {
// Shared by both extract_host() overloads: locates the [pos, end_pos) span
// of the host within a URL, using the exact same scan_authority() logic
// url::url() itself uses - so port ("host:80"), a trailing fragment with no
// path ("host#frag"), and userinfo containing a colon ("user:pass@host")
// are all handled identically here and in the full parser.
std::pair<size_t, size_t> find_host_bounds(std::string_view url) noexcept {
    size_t pos = url.find("://");
    pos = (pos != std::string::npos) ? pos + 3 : 0;
    const auto authority = scan_authority(url, pos);
    return {authority.host_start, authority.host_end};
}
}  // namespace

std::string urlparser::url::extract_host(std::string_view url) noexcept {
    const auto [pos, end_pos] = find_host_bounds(url);
    return std::string(url.substr(pos, end_pos - pos));
}

// A raw C-string literal has no allocation to hand over in the first place
// (it's static storage, not a heap buffer), so route it to the non-owning
// string_view overload above instead of letting it implicitly convert to a
// std::string and land in the string&& overload below. Without this,
// extract_host("literal") is ambiguous: a `const char*` converts equally
// well to std::string_view or to std::string, and those are two different,
// incomparable user-defined conversions from the compiler's point of view.
std::string urlparser::url::extract_host(const char* url) noexcept {
    return extract_host(std::string_view(url));
}

std::string urlparser::url::extract_host(std::string&& url) noexcept {
    // The caller owns url and is giving it up (rvalue), so its buffer is
    // already-allocated capacity we're free to reuse. Trimming both ends
    // with erase() keeps that same allocation - no new heap allocation for
    // the returned host string, unlike the string_view overload above which
    // must always allocate a fresh (smaller) string since it only has a
    // non-owning view to copy out of.
    const auto [pos, end_pos] = find_host_bounds(url);
    url.erase(end_pos);
    url.erase(0, pos);
    return std::move(url);
}

bool urlparser::url::operator==(const urlparser::url& other) const noexcept {
    return field(scheme_) == other.field(other.scheme_) &&
           field(userinfo_) == other.field(other.userinfo_) &&
           field(host_) == other.field(other.host_) && port_ == other.port_ &&
           field(path_) == other.field(other.path_) &&
           field(params_) == other.field(other.params_) &&
           field(query_) == other.field(other.query_) &&
           field(fragment_) == other.field(other.fragment_);
}

/// Builds (once, cached) the host for this URL: whichever of
/// hostname/ipv4/ipv6 it actually is. ignore_www is always passed as false
/// here since host_ has already had "www." stripped, at construction time,
/// if requested.
const urlparser::host& urlparser::url::ensure_host() const noexcept {
    if (!host_cache_) {
        host_cache_ = urlparser::host(field(host_), false);
    }
    return *host_cache_;
}

const urlparser::host& urlparser::url::host() const noexcept { return ensure_host(); }

std::ostream& operator<<(std::ostream& os, const urlparser::QueryParams& v) {
    os << "[";
    for (const auto& e : v) {
        os << e << ", ";
    }
    os << (v.empty() ? "" : "\b\b") << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const urlparser::url& dt) {
    os << dt.str();
    return os;
}

// ---------------------------------------------------------------------------
// host: Public-Suffix-List matching (suffix/domain/subdomain split).
// ---------------------------------------------------------------------------

// Defined here (not in urlparser.h) so the public header never needs to
// include the vendored src/ankerl/unordered_dense.h - see
// urlparser.h's forward declaration and src/ankerl/README.md.
class urlparser::detail::suffix_table {
   public:
    ankerl::unordered_dense::map<std::string, size_t> data;
};

urlparser::psl::psl() noexcept : levels_(std::make_unique<detail::suffix_table>()) {}
urlparser::psl::psl(psl&&) noexcept = default;
urlparser::psl& urlparser::psl::operator=(psl&&) noexcept = default;
urlparser::psl::~psl() = default;

bool urlparser::psl::is_loaded() const noexcept { return !levels_->data.empty(); }

urlparser::psl::psl(std::istream& stream) : levels_(std::make_unique<detail::suffix_table>()) {
    levels_->data.reserve(10'000);
    std::string line;
    size_t line_no = 0;
    while (std::getline(stream, line)) {
        if (version_.empty() && ++line_no <= 16) {
            constexpr std::string_view tag = "// VERSION:";
            if (line.compare(0, tag.size(), tag) == 0) {
                size_t start = line.find_first_not_of(' ', tag.size());
                if (start != std::string::npos) version_ = line.substr(start);
            }
        }

        // Only take up to the first whitespace.
        auto it = std::find_if(line.begin(), line.end(), [](char c) {
            return std::isspace(static_cast<unsigned char>(c));
        });
        line.resize(it - line.begin());

        if (line.empty()) continue;                  // blank line
        if (line.compare(0, 2, "//") == 0) continue;  // comment

        if (line[0] == '*') {
            if (line.size() <= 2 || line[1] != '.') {
                throw std::invalid_argument("Wildcard rule must be of form *.<host>");
            }
            add_rule(line, 1, 2);
        } else if (line[0] == '!') {
            if (line.size() <= 1) {
                throw std::invalid_argument("Exception rule has no hostname.");
            }
            add_rule(line, -1, 1);
        } else {
            add_rule(line, 0, 0);
        }
    }
}

/// The one, lazily-and-safely-initialized PSL instance, embedded at compile
/// time (see public_suffix_list_dat.h). A function-local static gives us
/// thread-safe, deferred-until-first-use initialization for free (unlike a
/// namespace-scope static, which would be subject to the usual static-
/// initialization-order-fiasco risk across translation units).
urlparser::psl& urlparser::psl::instance() noexcept {
    static psl the_instance = [] {
        std::stringstream stream{std::string(templates::public_suffix_list_dat)};
        return psl(stream);
    }();
    return the_instance;
}

std::string_view urlparser::psl::source_url() const noexcept { return URLPARSER_PUBLIC_SUFFIX_LIST_URL; }

void urlparser::psl::load_from_path(const std::string& filepath) {
    std::ifstream stream(filepath);
    if (!stream.good()) {
        throw std::invalid_argument("Path '" + filepath + "' is inaccessible.");
    }
    *this = psl(stream);
}

void urlparser::psl::load_from_string(const std::string& filestr) {
    std::stringstream stream(filestr);
    *this = psl(stream);
}

bool urlparser::psl::is_suffix(std::string_view text) const noexcept {
    if (text.empty()) return false;
    std::string reversed(text.rbegin(), text.rend());
    std::transform(reversed.begin(), reversed.end(), reversed.begin(), ascii_tolower);
    return levels_->data.find(reversed) != levels_->data.end();
}

/**
 * Get just the public suffix of a hostname. Works for either punycoded or
 * unpunycoded hostnames (but not mixed).
 *
 * Single pass: PSL rules are indexed by reversed text (see add_rule), so
 * matching walks a reversed+lowercased copy of hostname_text from its
 * most-specific label down toward the registrable-suffix boundary,
 * shrinking that same buffer in place (resize() to smaller never
 * reallocates) rather than re-scanning hostname_text a second time to
 * re-derive and re-copy the same substring, the way this used to be
 * split across suffix_length() + last_segments().
 */
std::string urlparser::psl::suffix_of(const std::string& hostname_text) const {
    std::string tld(hostname_text.rbegin(), hostname_text.rend());
    std::transform(tld.begin(), tld.end(), tld.begin(), ascii_tolower);

    while (!tld.empty()) {
        if (auto it = levels_->data.find(tld); it != levels_->data.end()) {
            // tld already *is* the matched suffix, reversed and
            // lowercased - reverse it back in place and we're done.
            std::reverse(tld.begin(), tld.end());
            return tld;
        }
        size_t position = tld.rfind('.');
        tld.resize((position == std::string::npos || position == 0) ? 0 : position);
    }

    // No rule matched at all, not even the bare top-level label (e.g. an
    // unrecognized/malformed TLD) - same fallback the old
    // suffix_length()==1 + last_segments(hostname_text, 1) combination
    // produced: just the last label of the original hostname.
    const size_t last_dot = hostname_text.rfind('.');
    std::string result = (last_dot == std::string::npos) ? hostname_text
                                                           : hostname_text.substr(last_dot + 1);
    std::transform(result.begin(), result.end(), result.begin(), ascii_tolower);
    return result;
}

size_t urlparser::psl::segment_count(const std::string& text) const {
    size_t count = 1;
    size_t position = text.find('.');
    while (position != std::string::npos) {
        count += 1;
        position = text.find('.', position + 1);
    }
    return count;
}

void urlparser::psl::add_rule(std::string& rule, int level_adjust, size_t trim) {
    std::string copy(rule.rbegin(), rule.rend() - trim);
    size_t length = segment_count(copy) + level_adjust;
    levels_->data[std::move(copy)] = length;
}

std::string_view urlparser::hostname::remove_www(const std::string_view& host) noexcept {
    if (host.compare(0, 4, "www.") != 0) {
        return host;
    }
    return host.substr(4);
}

urlparser::hostname::hostname(std::string host, const bool ignore_www)
    : host_(std::move(host)), ignore_www_(ignore_www) {}

urlparser::hostname urlparser::hostname::from_url(std::string_view url, const bool ignore_www) {
    return urlparser::hostname(urlparser::url::extract_host(url), ignore_www);
}

urlparser::hostname urlparser::hostname::from_url(const char* url, const bool ignore_www) {
    return from_url(std::string_view(url), ignore_www);
}

urlparser::hostname urlparser::hostname::from_url(const std::string& url, const bool ignore_www) {
    return urlparser::hostname(urlparser::url::extract_host(url), ignore_www);
}

urlparser::hostname urlparser::hostname::from_url(std::string&& url, const bool ignore_www) {
    return urlparser::hostname(urlparser::url::extract_host(std::move(url)), ignore_www);
}

void urlparser::hostname::ensure_parsed() const noexcept {
    if (parsed_) return;
    parsed_ = true;

    fulldomain_ = host_;
    suffix_ = urlparser::psl::instance().suffix_of(host_);
    // suffix_ is, by construction, always the tail of fulldomain_ (suffix_of()
    // matches against a reversed copy of host_, so whatever it returns is
    // exactly the trailing suffix.size() characters). So the position of the
    // '.' right before it is a direct arithmetic offset - no need to
    // allocate "." + suffix_ and rfind() it back out of fulldomain_.
    size_t suffix_pos = (suffix_.size() < fulldomain_.size())
                             ? fulldomain_.size() - suffix_.size() - 1
                             : std::string::npos;
    size_t subdomain_pos = 0;
    if (suffix_pos == std::string::npos || suffix_pos < 1) return;

    domain_ = host_.substr(0, suffix_pos);
    size_t domain_pos = domain_.find_last_of('.');
    if (domain_pos != std::string::npos) {
        if (ignore_www_) {
            size_t www_pos = domain_.find("www.");
            if (www_pos != 0) {
                if (www_pos != std::string::npos) return;
            } else {
                subdomain_pos = 4;  // length of "www."
                fulldomain_ = fulldomain_.substr(4);
            }
        }
        if (subdomain_pos < domain_pos) {
            subdomain_ = domain_.substr(subdomain_pos, domain_pos - subdomain_pos);
        }
        domain_ = domain_.substr(domain_pos + 1);
    }
}

const std::string& urlparser::hostname::suffix() const noexcept {
    ensure_parsed();
    return suffix_;
}

const std::string& urlparser::hostname::subdomain() const noexcept {
    ensure_parsed();
    return subdomain_;
}

const std::string& urlparser::hostname::domain() const noexcept {
    ensure_parsed();
    return domain_;
}

// Fast path: with ignore_www == false, full_domain is always exactly the
// original host string (the PSL-dependent split above never touches
// fulldomain_ in that case), so this skips the PSL lookup entirely for what
// is, by far, the most common call pattern.
const std::string& urlparser::hostname::full_domain() const noexcept {
    if (!ignore_www_) return host_;
    ensure_parsed();
    return fulldomain_;
}

const std::string& urlparser::hostname::str() const noexcept { return full_domain(); }

std::string urlparser::hostname::domain_name() const noexcept {
    ensure_parsed();
    return domain_ + "." + suffix_;
}

bool urlparser::hostname::operator==(const urlparser::hostname& other) const noexcept {
    return full_domain() == other.full_domain();
}

bool urlparser::hostname::operator==(const std::string& other) const noexcept {
    return full_domain() == other;
}

std::ostream& operator<<(std::ostream& os, const urlparser::hostname& dt) {
    os << dt.str();
    return os;
}

// --- ipv4 / ipv6 ------------------------------------------------------------

namespace {
// URLs write IPv6 host literals wrapped in brackets ("[::1]"); the parser
// below wants the bare address ("::1"). IPv4 addresses are never
// bracketed, so this is a no-op for them.
std::string_view strip_ip_brackets(std::string_view s) noexcept {
    if (s.size() >= 2 && s.front() == '[' && s.back() == ']') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

// Parses one dotted-quad octet ("0".."255", no sign, no leading zeros
// beyond a single "0") from [begin, end). Returns end-of-octet iterator
// on success, or `begin` (i.e. no progress) on failure - the empty range
// signals "not a valid octet" without needing a separate bool out-param.
std::string_view::const_iterator parse_ipv4_octet(std::string_view::const_iterator begin,
                                                    std::string_view::const_iterator end,
                                                    uint8_t& out) noexcept {
    if (begin == end || !std::isdigit(static_cast<unsigned char>(*begin))) return begin;
    if (*begin == '0') {
        out = 0;
        return begin + 1;  // "0" is valid; "00", "01" (leading zeros) are not
    }
    unsigned value = 0;
    auto it = begin;
    for (; it != end && std::isdigit(static_cast<unsigned char>(*it)) && it - begin < 3; ++it) {
        value = value * 10 + static_cast<unsigned>(*it - '0');
    }
    if (value > 255) return begin;
    out = static_cast<uint8_t>(value);
    return it;
}

// Strict dotted-quad IPv4 parser: exactly 4 octets, each 0-255, separated
// by literal '.', nothing else in the string (no leading/trailing junk,
// no whitespace, no alternate bases like the octal/hex forms some libc
// inet_aton()s historically accepted).
bool parse_ipv4(std::string_view text, std::array<uint8_t, 4>& out) noexcept {
    auto it = text.begin();
    const auto end = text.end();
    for (int i = 0; i < 4; ++i) {
        if (i > 0) {
            if (it == end || *it != '.') return false;
            ++it;
        }
        auto next = parse_ipv4_octet(it, end, out[i]);
        if (next == it) return false;
        it = next;
    }
    return it == end;
}

std::string format_ipv4(const std::array<uint8_t, 4>& bytes) {
    std::string out;
    out.reserve(15);
    for (int i = 0; i < 4; ++i) {
        if (i > 0) out += '.';
        out += std::to_string(bytes[i]);
    }
    return out;
}

// Parses one IPv6 hextet (1-4 hex digits) from [begin, end) into a 16-bit
// value. Same no-progress-on-failure convention as parse_ipv4_octet.
std::string_view::const_iterator parse_ipv6_hextet(std::string_view::const_iterator begin,
                                                     std::string_view::const_iterator end,
                                                     uint16_t& out) noexcept {
    unsigned value = 0;
    auto it = begin;
    for (; it != end && std::isxdigit(static_cast<unsigned char>(*it)) && it - begin < 4; ++it) {
        char c = *it;
        unsigned digit = std::isdigit(static_cast<unsigned char>(c))
                              ? static_cast<unsigned>(c - '0')
                              : static_cast<unsigned>(std::tolower(static_cast<unsigned char>(c)) - 'a' + 10);
        value = (value << 4) | digit;
    }
    if (it == begin) return begin;
    out = static_cast<uint16_t>(value);
    return it;
}

// Full IPv6 text-form parser: 8 colon-separated hextets, with an optional
// single "::" run standing in for one or more all-zero hextets, and an
// optional trailing embedded IPv4 ("::ffff:192.0.2.1"). Splits on the
// (at most one) "::" first, then parses each side independently and
// zero-fills the gap - the standard approach for this grammar.
bool parse_ipv6(std::string_view text, std::array<uint8_t, 16>& out) noexcept {
    if (text.size() < 2) return false;

    size_t compress_pos = text.find("::");
    if (compress_pos != std::string_view::npos && text.find("::", compress_pos + 1) != std::string_view::npos) {
        return false;  // "::" may appear at most once
    }

    std::string_view left = compress_pos == std::string_view::npos ? text : text.substr(0, compress_pos);
    std::string_view right =
        compress_pos == std::string_view::npos ? std::string_view{} : text.substr(compress_pos + 2);

    // Parses a run of ':'-separated hextets into `groups` (max 8), with
    // support for one trailing embedded IPv4 literal (counts as 2
    // hextets). Returns false on any malformed content.
    auto parse_side = [](std::string_view side, std::array<uint16_t, 8>& groups, int& count) noexcept -> bool {
        count = 0;
        if (side.empty()) return true;
        auto it = side.begin();
        const auto end = side.end();
        while (true) {
            // Embedded IPv4 tail: only valid as the last group, detected
            // by a '.' appearing before the next ':'.
            auto colon_or_end = std::find(it, end, ':');
            if (std::find(it, colon_or_end, '.') != colon_or_end) {
                std::array<uint8_t, 4> v4{};
                if (!parse_ipv4(std::string_view(&*it, static_cast<size_t>(end - it)), v4)) return false;
                if (count > 6) return false;
                groups[count++] = static_cast<uint16_t>((v4[0] << 8) | v4[1]);
                groups[count++] = static_cast<uint16_t>((v4[2] << 8) | v4[3]);
                it = end;
                break;
            }
            uint16_t hextet = 0;
            auto next = parse_ipv6_hextet(it, colon_or_end, hextet);
            if (next != colon_or_end || count >= 8) return false;
            groups[count++] = hextet;
            it = colon_or_end;
            if (it == end) break;
            ++it;                 // skip ':'
            if (it == end) return false;  // trailing lone ':'
        }
        return true;
    };

    std::array<uint16_t, 8> left_groups{}, right_groups{};
    int left_count = 0, right_count = 0;
    if (!parse_side(left, left_groups, left_count)) return false;
    if (!parse_side(right, right_groups, right_count)) return false;

    int total = left_count + right_count;
    if (compress_pos == std::string_view::npos) {
        if (total != 8) return false;
    } else {
        if (total >= 8) return false;  // "::" must stand in for >=1 hextet
    }

    std::array<uint16_t, 8> groups{};
    for (int i = 0; i < left_count; ++i) groups[i] = left_groups[i];
    for (int i = 0; i < right_count; ++i) groups[8 - right_count + i] = right_groups[i];

    for (int i = 0; i < 8; ++i) {
        out[2 * i] = static_cast<uint8_t>(groups[i] >> 8);
        out[2 * i + 1] = static_cast<uint8_t>(groups[i] & 0xFF);
    }
    return true;
}

// Formats per RFC 5952: lowercase hex, the longest run of >=2 zero
// hextets (leftmost wins on ties) compressed to "::", no leading zeros
// within a hextet.
std::string format_ipv6(const std::array<uint8_t, 16>& bytes) {
    std::array<uint16_t, 8> groups{};
    for (int i = 0; i < 8; ++i) {
        groups[i] = static_cast<uint16_t>((bytes[2 * i] << 8) | bytes[2 * i + 1]);
    }

    int best_start = -1, best_len = 0;
    for (int i = 0; i < 8;) {
        if (groups[i] != 0) {
            ++i;
            continue;
        }
        int j = i;
        while (j < 8 && groups[j] == 0) ++j;
        if (j - i > best_len) {
            best_len = j - i;
            best_start = i;
        }
        i = j;
    }
    if (best_len < 2) best_start = -1;  // RFC 5952: don't compress a single zero group

    std::string out;
    out.reserve(39);
    char buf[5];
    for (int i = 0; i < 8;) {
        if (i == best_start) {
            out += "::";
            i += best_len;
            continue;
        }
        if (!out.empty() && out.back() != ':') out += ':';
        int n = std::snprintf(buf, sizeof(buf), "%x", groups[i]);
        out.append(buf, static_cast<size_t>(n));
        ++i;
    }
    if (out.empty()) out = "::";
    return out;
}

uint64_t be_bytes_to_u64(const uint8_t* p) noexcept {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | p[i];
    return v;
}

void u64_to_be_bytes(uint64_t v, uint8_t* p) noexcept {
    for (int i = 7; i >= 0; --i) {
        p[i] = static_cast<uint8_t>(v & 0xFF);
        v >>= 8;
    }
}
}  // namespace

// --- ipv4 --------------------------------------------------------------

bool urlparser::ipv4::is_valid(std::string_view text) noexcept {
    std::array<uint8_t, 4> bytes{};
    return parse_ipv4(text, bytes);
}

urlparser::ipv4::ipv4(std::string_view text) {
    std::array<uint8_t, 4> bytes{};
    if (parse_ipv4(text, bytes)) {
        value_ = (static_cast<uint32_t>(bytes[0]) << 24) | (static_cast<uint32_t>(bytes[1]) << 16) |
                 (static_cast<uint32_t>(bytes[2]) << 8) | static_cast<uint32_t>(bytes[3]);
        return;
    }
    throw std::invalid_argument("ipv4: not a valid IPv4 address: " + std::string(text));
}

urlparser::ipv4 urlparser::ipv4::from_uint32(uint32_t address) noexcept { return ipv4(address); }

urlparser::ipv4 urlparser::ipv4::from_url(std::string_view url_text) {
    return ipv4(std::string_view(urlparser::url::extract_host(url_text)));
}

std::array<uint8_t, 4> urlparser::ipv4::bytes() const noexcept {
    return {static_cast<uint8_t>(value_ >> 24), static_cast<uint8_t>(value_ >> 16),
            static_cast<uint8_t>(value_ >> 8), static_cast<uint8_t>(value_)};
}

std::string urlparser::ipv4::str() const { return format_ipv4(bytes()); }

std::ostream& operator<<(std::ostream& os, const urlparser::ipv4& dt) {
    os << dt.str();
    return os;
}

// --- ipv6 --------------------------------------------------------------

bool urlparser::ipv6::is_valid(std::string_view text) noexcept {
    std::array<uint8_t, 16> bytes{};
    return parse_ipv6(strip_ip_brackets(text), bytes);
}

urlparser::ipv6::ipv6(std::string_view text) {
    std::array<uint8_t, 16> bytes{};
    if (parse_ipv6(strip_ip_brackets(text), bytes)) {
        hi_ = be_bytes_to_u64(bytes.data());
        lo_ = be_bytes_to_u64(bytes.data() + 8);
        return;
    }
    throw std::invalid_argument("ipv6: not a valid IPv6 address: " + std::string(text));
}

urlparser::ipv6 urlparser::ipv6::from_uint64_pair(uint64_t high, uint64_t low) noexcept {
    return ipv6(high, low);
}

urlparser::ipv6 urlparser::ipv6::from_url(std::string_view url_text) {
    return ipv6(std::string_view(urlparser::url::extract_host(url_text)));
}

std::array<uint8_t, 16> urlparser::ipv6::bytes() const noexcept {
    std::array<uint8_t, 16> out{};
    u64_to_be_bytes(hi_, out.data());
    u64_to_be_bytes(lo_, out.data() + 8);
    return out;
}

std::string urlparser::ipv6::str() const { return format_ipv6(bytes()); }

void urlparser::ipv6::add_bits(uint64_t low64_bits) noexcept {
    const uint64_t old_lo = lo_;
    lo_ += low64_bits;
    const bool carried = lo_ < old_lo;
    const bool negative = (low64_bits >> 63) & 1;
    hi_ += static_cast<uint64_t>(carried) + (negative ? ~uint64_t{0} : 0);
}

std::ostream& operator<<(std::ostream& os, const urlparser::ipv6& dt) {
    os << dt.str();
    return os;
}

// --- host classification (hostname / ipv4 / ipv6) -----------------------

urlparser::host::host(std::string_view host_text, bool ignore_www) noexcept {
    if (ipv4::is_valid(host_text)) {
        value_ = ipv4(host_text);
    } else if (ipv6::is_valid(host_text)) {
        value_ = ipv6(host_text);
    } else {
        value_ = hostname(std::string(host_text), ignore_www);
    }
}

urlparser::host::host(const char* host_text, bool ignore_www) noexcept
    : host(std::string_view(host_text), ignore_www) {}

urlparser::host::host(std::string&& host_text, bool ignore_www) noexcept {
    // is_valid() only reads host_text - checking IP-ness first, before
    // deciding whether to consume it, is what lets the hostname branch
    // below still steal the buffer via std::move().
    if (ipv4::is_valid(host_text)) {
        value_ = ipv4(host_text);
    } else if (ipv6::is_valid(host_text)) {
        value_ = ipv6(host_text);
    } else {
        value_ = hostname(std::move(host_text), ignore_www);
    }
}

urlparser::host urlparser::host::from_url(std::string_view url_text, bool ignore_www) noexcept {
    return host(urlparser::url::extract_host(url_text), ignore_www);
}

urlparser::host urlparser::host::from_url(const char* url_text, bool ignore_www) noexcept {
    return from_url(std::string_view(url_text), ignore_www);
}

urlparser::host urlparser::host::from_url(std::string&& url_text, bool ignore_www) noexcept {
    // extract_host(string&&) reuses url_text's own buffer (erase() in
    // place) instead of allocating a new host-sized string; the
    // host(string&&) constructor then reuses *that* buffer again for the
    // hostname case - so a URL string the caller already owns and doesn't
    // need afterward goes from "URL" to classified "hostname" with zero
    // new allocations beyond the host object itself (none - it just takes
    // ownership of the buffer it's handed).
    return host(urlparser::url::extract_host(std::move(url_text)), ignore_www);
}

std::string urlparser::host::str() const {
    return std::visit([](const auto& v) -> std::string { return v.str(); }, value_);
}

std::ostream& operator<<(std::ostream& os, const urlparser::host& dt) {
    os << dt.str();
    return os;
}
