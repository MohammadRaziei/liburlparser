//
// url + host: parsing, string reconstruction, and Public-Suffix-List
// matching - everything liburlparser's core does, in one file. (Previously
// split across url.h + url.cpp + urlparser_url.cpp + psl.h + psl.cpp +
// urlparser_host.cpp, with a confusing detail::url class wrapped by another
// url::Impl, and a PIMPL host::Impl wrapping a separately-loaded PSL.) url
// and host are now flat value types declared directly in
// include/urlparser.h - there is nothing to wrap here anymore, just the
// method bodies. PSL (the one piece that genuinely needs a
// std::unordered_map) lives entirely here, in urlparser::detail, and never
// appears in any header at all.
//
#include "urlparser.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "public_suffix_list_dat.h"

namespace {

// ::tolower goes through the current C locale on every call; URLs are
// ASCII-only per RFC 3986 (IDNs arrive already punycode-encoded), so a
// branchless ASCII-only version is both correct and several times faster.
inline char ascii_tolower(char c) noexcept {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
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
AuthorityBounds scan_authority(std::string_view url, size_t position) noexcept {
    size_t authority_end = url.length();
    size_t at_pos = std::string::npos;
    size_t colon_pos = std::string::npos;
    for (size_t i = position; i < url.length(); ++i) {
        char c = url[i];
        if (c == '/' || c == '?' || c == '#') {
            authority_end = i;
            break;
        }
        if (c == '@') {
            at_pos = i;
            colon_pos = std::string::npos;  // a ':' before '@' is userinfo, not a port
        } else if (c == ':' && colon_pos == std::string::npos) {
            colon_pos = i;
        }
    }
    const size_t host_start = (at_pos != std::string::npos) ? at_pos + 1 : position;
    const size_t host_end = (colon_pos != std::string::npos) ? colon_pos : authority_end;
    return {host_start, host_end, at_pos, colon_pos, authority_end};
}

const std::unordered_set<std::string> USES_NETLOC = {
    "",     "file",  "ftp",   "git",   "git+ssh", "gopher", "http",
    "https", "imap", "mms",   "nfs",   "nntp",    "prospero", "rsync",
    "rtsp", "rtspu", "sftp",  "shttp", "snews",   "svn",    "svn+ssh",
    "telnet", "wais"};

const std::unordered_set<std::string> USES_PARAMS = {
    "",   "ftp",  "hdl",   "http", "https", "imap", "mms",
    "prospero", "rtsp", "rtspu", "sftp", "shttp", "sip", "sips", "tel"};

const std::unordered_set<std::string> KNOWN_PROTOCOLS = {
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

bool urlparser::url::is_psl_loaded() noexcept { return urlparser::host::is_psl_loaded(); }

urlparser::url::url(std::string_view url, const bool ignore_www)
    : ignore_www_(ignore_www) {
    // Every field is a non-expanding substring of url (case-folding only
    // changes case, never length), so their combined length can never
    // exceed url.size() - reserving exactly that (plus the configurable
    // headroom, for future setters) means storage_ never reallocates
    // during parsing, so the Spans we hand out below stay valid forever.
    storage_.reserve(url.size() + URLPARSER_ARENA_EXTRA_CAPACITY);

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
                if (KNOWN_PROTOCOLS.find(std::string(field(scheme_))) != KNOWN_PROTOCOLS.end()) {
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

    // Single forward pass over the rest of the URL to locate the first '#'
    // (fragment), the first '?' before it (query), and - only for schemes
    // that use params - the first ';' before the query.
    const bool track_params = USES_PARAMS.find(std::string(field(scheme_))) != USES_PARAMS.end();
    size_t hash_pos = std::string::npos;
    size_t query_pos = std::string::npos;
    size_t params_pos = std::string::npos;

    for (size_t i = position; i < url.length(); ++i) {
        char c = url[i];
        if (c == '#') {
            hash_pos = i;
            break;
        }
        if (query_pos == std::string::npos && c == '?') {
            query_pos = i;
        } else if (track_params && params_pos == std::string::npos &&
                   query_pos == std::string::npos && c == ';') {
            params_pos = i;
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
        result.append(USES_NETLOC.find(std::string(scheme)) == USES_NETLOC.end() ? ":" : "://");
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

/// Builds (once, cached) the host for this URL. ignore_www is always passed
/// as false here since host_ has already had "www." stripped, at
/// construction time, if requested.
const urlparser::host& urlparser::url::ensure_host() const noexcept {
    if (!host_cache_) {
        host_cache_.emplace(std::string(field(host_)), false);
    }
    return *host_cache_;
}

const urlparser::host& urlparser::url::host() const noexcept { return ensure_host(); }
const std::string& urlparser::url::suffix() const noexcept { return ensure_host().suffix(); }
const std::string& urlparser::url::subdomain() const noexcept { return ensure_host().subdomain(); }
const std::string& urlparser::url::domain() const noexcept { return ensure_host().domain(); }
std::string urlparser::url::domain_name() const noexcept { return ensure_host().domain_name(); }

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

namespace urlparser::detail {

/**
 * Finds the TLD (public suffix) of a hostname according to a Public Suffix
 * List. Internal to this file - host is the only thing that uses it.
 */
struct PSL {
    PSL() = default;
    explicit PSL(std::istream& stream) {
        levels.reserve(10'000);
        std::string line;
        while (std::getline(stream, line)) {
            // Only take up to the first whitespace.
            auto it = std::find_if(line.begin(), line.end(), [](char c) {
                return std::isspace(static_cast<unsigned char>(c));
            });
            line.resize(it - line.begin());

            if (line.empty()) continue;              // blank line
            if (line.compare(0, 2, "//") == 0) continue;  // comment

            if (line[0] == '*') {
                if (line.size() <= 2 || line[1] != '.') {
                    throw std::invalid_argument("Wildcard rule must be of form *.<host>");
                }
                add(line, 1, 2);
            } else if (line[0] == '!') {
                if (line.size() <= 1) {
                    throw std::invalid_argument("Exception rule has no hostname.");
                }
                add(line, -1, 1);
            } else {
                add(line, 0, 0);
            }
        }
    }

    static PSL fromPath(const std::string& path) {
        std::ifstream stream(path);
        if (!stream.good()) {
            throw std::invalid_argument("Path '" + path + "' is inaccessible.");
        }
        return PSL(stream);
    }

    static PSL fromString(const std::string& str) {
        std::stringstream stream(str);
        return PSL(stream);
    }

    /**
     * Get just the TLD (public suffix) of the hostname. Works for either
     * punycoded or unpunycoded hostnames (but not mixed).
     */
    std::string getTLD(const std::string& hostname) const {
        return getLastSegments(hostname, getTLDLength(hostname));
    }

    size_t numLevels() const noexcept { return levels.size(); }

   private:
    // Mapping of a reversed rule string to its level (segment count).
    std::unordered_map<std::string, size_t> levels;

    size_t countSegments(const std::string& hostname) const {
        size_t count = 1;
        size_t position = hostname.find('.');
        while (position != std::string::npos) {
            count += 1;
            position = hostname.find('.', position + 1);
        }
        return count;
    }

    size_t getTLDLength(const std::string& hostname) const {
        std::string tld(hostname.rbegin(), hostname.rend());
        std::transform(tld.begin(), tld.end(), tld.begin(), ascii_tolower);

        while (!tld.empty()) {
            if (auto it = levels.find(tld); it != levels.end()) {
                return it->second;
            }
            size_t position = tld.rfind('.');
            tld.resize((position == std::string::npos || position == 0) ? 0 : position);
        }
        return 1;
    }

    std::string getLastSegments(const std::string& hostname, size_t segments) const {
        size_t position = hostname.size();
        size_t remaining = segments;
        while (remaining != 0 && position && position != std::string::npos) {
            position = hostname.rfind('.', position - 1);
            remaining -= 1;
        }
        if (remaining >= 1) return "";

        const size_t start = (position == std::string::npos) ? 0 : position + 1;
        std::string result(hostname, start);
        std::transform(result.begin(), result.end(), result.begin(), ascii_tolower);

        if (!result.empty() && result[0] == '.') {
            throw std::invalid_argument("Empty segment in " + result);
        }
        return result;
    }

    void add(std::string& rule, int level_adjust, size_t trim) {
        std::string copy(rule.rbegin(), rule.rend() - trim);
        size_t length = countSegments(copy) + level_adjust;
        levels[std::move(copy)] = length;
    }
};

/// The one, lazily-and-safely-initialized PSL instance, embedded at compile
/// time (see public_suffix_list_dat.h). A function-local static gives us
/// thread-safe, deferred-until-first-use initialization for free (unlike a
/// namespace-scope static, which would be subject to the usual static-
/// initialization-order-fiasco risk across translation units).
PSL& psl() {
    static PSL instance = PSL::fromString(templates::public_suffix_list_dat);
    return instance;
}

}  // namespace urlparser::detail

void urlparser::host::load_psl_from_path(const std::string& filepath) {
    urlparser::detail::psl() = urlparser::detail::PSL::fromPath(filepath);
}

void urlparser::host::load_psl_from_string(const std::string& filestr) {
    urlparser::detail::psl() = urlparser::detail::PSL::fromString(filestr);
}

bool urlparser::host::is_psl_loaded() noexcept {
    return urlparser::detail::psl().numLevels() > 0;
}

std::string_view urlparser::host::remove_www(const std::string_view& host) noexcept {
    if (host.compare(0, 4, "www.") != 0) {
        return host;
    }
    return host.substr(4);
}

urlparser::host::host(std::string host, const bool ignore_www)
    : host_(std::move(host)), ignore_www_(ignore_www) {}

urlparser::host urlparser::host::from_url(std::string_view url, const bool ignore_www) {
    return urlparser::host(urlparser::url::extract_host(url), ignore_www);
}

urlparser::host urlparser::host::from_url(const char* url, const bool ignore_www) {
    return from_url(std::string_view(url), ignore_www);
}

urlparser::host urlparser::host::from_url(const std::string& url, const bool ignore_www) {
    return urlparser::host(urlparser::url::extract_host(url), ignore_www);
}

urlparser::host urlparser::host::from_url(std::string&& url, const bool ignore_www) {
    return urlparser::host(urlparser::url::extract_host(std::move(url)), ignore_www);
}

void urlparser::host::ensure_parsed() const noexcept {
    if (parsed_) return;
    parsed_ = true;

    fulldomain_ = host_;
    suffix_ = urlparser::detail::psl().getTLD(host_);
    size_t suffix_pos = fulldomain_.rfind("." + suffix_);
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

const std::string& urlparser::host::suffix() const noexcept {
    ensure_parsed();
    return suffix_;
}

const std::string& urlparser::host::subdomain() const noexcept {
    ensure_parsed();
    return subdomain_;
}

const std::string& urlparser::host::domain() const noexcept {
    ensure_parsed();
    return domain_;
}

// Fast path: with ignore_www == false, full_domain is always exactly the
// original host string (the PSL-dependent split above never touches
// fulldomain_ in that case), so this skips the PSL lookup entirely for what
// is, by far, the most common call pattern.
const std::string& urlparser::host::full_domain() const noexcept {
    if (!ignore_www_) return host_;
    ensure_parsed();
    return fulldomain_;
}

const std::string& urlparser::host::str() const noexcept { return full_domain(); }

std::string urlparser::host::domain_name() const noexcept {
    ensure_parsed();
    return domain_ + "." + suffix_;
}

bool urlparser::host::operator==(const urlparser::host& other) const noexcept {
    return full_domain() == other.full_domain();
}

bool urlparser::host::operator==(const std::string& other) const noexcept {
    return full_domain() == other;
}

std::ostream& operator<<(std::ostream& os, const urlparser::host& dt) {
    os << dt.str();
    return os;
}
