//
// url + host: parsing, string reconstruction, and Public-Suffix-List
// matching - everything liburlparser's core does, in one file. (Previously
// split across url.h + url.cpp + urlparser_url.cpp + psl.h + psl.cpp +
// urlparser_host.cpp, with a confusing detail::url class wrapped by another
// url::Impl, and a PIMPL host::Impl wrapping a separately-loaded PSL.) url,
// hostname, and psl are now flat value types declared directly in
// include/urlparser.h - there is nothing to wrap here anymore, just the
// method bodies. psl's std::unordered_map is a plain private member, same
// as any other class's private state - no detail namespace, no PIMPL.
//
#include "urlparser.h"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

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

urlparser::psl::psl(std::istream& stream) {
    levels_.reserve(10'000);
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

std::string_view urlparser::psl::source_url() const noexcept { return PUBLIC_SUFFIX_LIST_URL; }

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
    return levels_.find(reversed) != levels_.end();
}

/**
 * Get just the public suffix of a hostname. Works for either punycoded or
 * unpunycoded hostnames (but not mixed).
 */
std::string urlparser::psl::suffix_of(const std::string& hostname_text) const {
    return last_segments(hostname_text, suffix_length(hostname_text));
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

size_t urlparser::psl::suffix_length(const std::string& hostname_text) const {
    std::string tld(hostname_text.rbegin(), hostname_text.rend());
    std::transform(tld.begin(), tld.end(), tld.begin(), ascii_tolower);

    while (!tld.empty()) {
        if (auto it = levels_.find(tld); it != levels_.end()) {
            return it->second;
        }
        size_t position = tld.rfind('.');
        tld.resize((position == std::string::npos || position == 0) ? 0 : position);
    }
    return 1;
}

std::string urlparser::psl::last_segments(const std::string& hostname_text, size_t segments) const {
    size_t position = hostname_text.size();
    size_t remaining = segments;
    while (remaining != 0 && position && position != std::string::npos) {
        position = hostname_text.rfind('.', position - 1);
        remaining -= 1;
    }
    if (remaining >= 1) return "";

    const size_t start = (position == std::string::npos) ? 0 : position + 1;
    std::string result(hostname_text, start);
    std::transform(result.begin(), result.end(), result.begin(), ascii_tolower);

    if (!result.empty() && result[0] == '.') {
        throw std::invalid_argument("Empty segment in " + result);
    }
    return result;
}

void urlparser::psl::add_rule(std::string& rule, int level_adjust, size_t trim) {
    std::string copy(rule.rbegin(), rule.rend() - trim);
    size_t length = segment_count(copy) + level_adjust;
    levels_[std::move(copy)] = length;
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
// URLs write IPv6 host literals wrapped in brackets ("[::1]"); inet_pton
// wants the bare address ("::1"). IPv4 addresses are never bracketed, so
// this is a no-op for them.
std::string_view strip_ip_brackets(std::string_view s) noexcept {
    if (s.size() >= 2 && s.front() == '[' && s.back() == ']') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

// inet_pton/InetPtonA need a NUL-terminated C string; string_view isn't
// guaranteed one. INET6_ADDRSTRLEN (46) is long enough for any valid
// address (including a fully-expanded IPv6 one), so anything longer than
// that can't possibly be valid and is rejected up front instead of being
// silently truncated into the buffer.
bool make_c_string(std::string_view s, char (&buf)[INET6_ADDRSTRLEN]) noexcept {
    if (s.empty() || s.size() >= sizeof(buf)) return false;
    std::memcpy(buf, s.data(), s.size());
    buf[s.size()] = '\0';
    return true;
}

#ifdef _WIN32
int platform_inet_pton(int af, const char* src, void* dst) { return InetPtonA(af, src, dst); }
const char* platform_inet_ntop(int af, const void* src, char* dst, size_t size) {
    return InetNtopA(af, const_cast<void*>(src), dst, size);
}
#else
int platform_inet_pton(int af, const char* src, void* dst) { return inet_pton(af, src, dst); }
const char* platform_inet_ntop(int af, const void* src, char* dst, size_t size) {
    return inet_ntop(af, src, dst, size);
}
#endif

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
    char buf[INET6_ADDRSTRLEN];
    std::array<uint8_t, 4> bytes{};
    return make_c_string(text, buf) && platform_inet_pton(AF_INET, buf, bytes.data()) == 1;
}

urlparser::ipv4::ipv4(std::string_view text) {
    char buf[INET6_ADDRSTRLEN];
    std::array<uint8_t, 4> bytes{};
    if (make_c_string(text, buf) && platform_inet_pton(AF_INET, buf, bytes.data()) == 1) {
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

std::string urlparser::ipv4::str() const {
    char buf[INET6_ADDRSTRLEN];
    const auto b = bytes();
    // bytes() always comes from a validated parse or from_uint32(), so
    // inet_ntop failing here isn't a real-world case - the API is noexcept
    // (well, str() itself may still allocate but never throws for reasons
    // other than allocation failure), so fail safe (empty string) rather
    // than use an unwritten buffer.
    if (!platform_inet_ntop(AF_INET, b.data(), buf, sizeof(buf))) return {};
    return std::string(buf);
}

std::ostream& operator<<(std::ostream& os, const urlparser::ipv4& dt) {
    os << dt.str();
    return os;
}

// --- ipv6 --------------------------------------------------------------

bool urlparser::ipv6::is_valid(std::string_view text) noexcept {
    char buf[INET6_ADDRSTRLEN];
    std::array<uint8_t, 16> bytes{};
    return make_c_string(strip_ip_brackets(text), buf) &&
           platform_inet_pton(AF_INET6, buf, bytes.data()) == 1;
}

urlparser::ipv6::ipv6(std::string_view text) {
    char buf[INET6_ADDRSTRLEN];
    std::array<uint8_t, 16> bytes{};
    if (make_c_string(strip_ip_brackets(text), buf) &&
        platform_inet_pton(AF_INET6, buf, bytes.data()) == 1) {
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

std::string urlparser::ipv6::str() const {
    char buf[INET6_ADDRSTRLEN];
    const auto b = bytes();
    if (!platform_inet_ntop(AF_INET6, b.data(), buf, sizeof(buf))) return {};
    return std::string(buf);
}

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
