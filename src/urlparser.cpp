//
// Url: parsing + string reconstruction.
//
// This is the single file for everything URL-string-related (previously
// split across url.h + url.cpp + urlparser_url.cpp, with a confusing
// detail::Url class wrapped by another Url::Impl class). Url is now a flat
// value type declared directly in include/urlparser.h - there is nothing to
// wrap here anymore, just the method bodies.
//
#include "urlparser.h"

#include <algorithm>
#include <charconv>
#include <unordered_set>

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

bool urlparser::Url::isPslLoaded() noexcept { return urlparser::Host::isPslLoaded(); }

urlparser::Url::Url(const std::string& url, const bool ignore_www)
    : ignore_www_(ignore_www) {
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
                scheme_.assign(url, 0, index);
                std::transform(scheme_.begin(), scheme_.end(), scheme_.begin(),
                                ascii_tolower);
                position = index + 1;
            } else {
                scheme_.assign(url, 0, index);
                std::transform(scheme_.begin(), scheme_.end(), scheme_.begin(),
                                ascii_tolower);
                if (KNOWN_PROTOCOLS.find(scheme_) != KNOWN_PROTOCOLS.end()) {
                    position = index + 1;
                } else {
                    scheme_.clear();
                }
            }
        }
    }

    // Search for the netloc.
    if ((url.length() - position) >= 1 && url[position] == '/' &&
        url[position + 1] == '/') {
        // Skip the '//'.
        position += 2;

        // Single forward pass over the authority section: find where it ends
        // (first '/', '?' or '#') and, within that range, the userinfo '@'
        // separator and the port ':' separator - all in one scan.
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

        if (at_pos != std::string::npos) {
            userinfo_.assign(url, position, at_pos - position);
        }

        host_.assign(url, host_start, host_end - host_start);
        std::transform(host_.begin(), host_.end(), host_.begin(), ascii_tolower);
        if (ignore_www_) {
            // Cheap (a prefix check, not a PSL lookup) - do it once, here,
            // rather than lazily: host_ is then a plain, immutable-after-
            // construction field like everything else, and str()/
            // fulldomain()/host() all agree with each other from the start.
            host_ = std::string(urlparser::Host::removeWWW(host_));
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
                const std::string portText(first, last);
                if (ec != std::errc() || ptr != last) {
                    throw std::invalid_argument("Port not a number: " + portText);
                }
                if (parsed_port > 65535 || parsed_port < 0) {
                    throw std::invalid_argument("Port out of range: " + portText);
                }
                port_ = parsed_port;
            }
        }
    }

    // Single forward pass over the rest of the URL to locate the first '#'
    // (fragment), the first '?' before it (query), and - only for schemes
    // that use params - the first ';' before the query.
    const bool track_params = USES_PARAMS.find(scheme_) != USES_PARAMS.end();
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

    path_.assign(url, position, path_content_end - position);

    if (params_pos != std::string::npos) {
        params_.assign(url, params_pos + 1, path_end - params_pos - 1);
        has_params_ = true;
    }

    if (query_pos != std::string::npos) {
        const size_t query_end = (hash_pos != std::string::npos) ? hash_pos : url.length();
        query_.assign(url, query_pos + 1, query_end - query_pos - 1);
        has_query_ = true;
    }

    if (hash_pos != std::string::npos) {
        fragment_.assign(url, hash_pos + 1, url.length() - hash_pos - 1);
    }
}

std::string urlparser::Url::str() const noexcept {
    std::string result;

    if (!scheme_.empty()) {
        result.append(scheme_);
        result.append(USES_NETLOC.find(scheme_) == USES_NETLOC.end() ? ":" : "://");
    } else if (!host_.empty()) {
        result.append("//");
    }

    if (!userinfo_.empty()) {
        result.append(userinfo_);
        result.append("@");
    }

    if (!host_.empty()) {
        result.append(host_);
    }

    if (port_) {
        result.append(":");
        result.append(std::to_string(port_));
    }

    if (path_.empty()) {
        if (!result.empty()) {
            result.append("/");
        }
    } else {
        if (!host_.empty() && path_[0] != '/') {
            result.append(1, '/');
        }
        result.append(path_);
    }

    if (has_params_) {
        result.append(";");
        result.append(params_);
    }

    if (has_query_) {
        result.append("?");
        result.append(query_);
    }

    if (!fragment_.empty()) {
        result.append("#");
        result.append(fragment_);
    }

    return result;
}

std::string urlparser::Url::abspath() const noexcept {
    // Resolves '.'/'..' path segments, the way a filesystem path resolver
    // would - a pure computation (unlike the old chainable UrlImpl::abspath(),
    // which mutated path_ in place as a side effect of what looked like a
    // read-only getter).
    std::string result;
    std::vector<size_t> segment_starts;

    if (!path_.empty() && path_[0] == '/') {
        result.append(1, '/');
        segment_starts.push_back(0);
    }

    size_t previous = 0;
    size_t index = path_.find('/');
    auto emit_segment = [&](size_t start, size_t end) {
        if (end - start == 0) return;  // skip empty segments
        const std::string_view segment(path_.data() + start, end - start);
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

    for (; index != std::string::npos; previous = index + 1, index = path_.find('/', index + 1)) {
        emit_segment(previous, index);
    }
    emit_segment(previous, path_.size());

    return result;
}

urlparser::QueryParams urlparser::Url::params() const noexcept { return split(query_, '&'); }

std::string urlparser::Url::extractHost(const std::string& url) noexcept {
    size_t pos = url.find("://");
    pos = (pos != std::string::npos) ? pos + 3 : 0;
    size_t end_pos = url.find_first_of("?/", pos);
    if (end_pos == std::string::npos) {
        end_pos = url.length();
    }
    if (size_t at_pos = url.find_first_of('@', pos); at_pos < end_pos) {
        pos = at_pos + 1;
    }
    return url.substr(pos, end_pos - pos);
}

bool urlparser::Url::operator==(const urlparser::Url& other) const noexcept {
    return scheme_ == other.scheme_ && userinfo_ == other.userinfo_ &&
           host_ == other.host_ && port_ == other.port_ && path_ == other.path_ &&
           params_ == other.params_ && query_ == other.query_ &&
           fragment_ == other.fragment_;
}

/// Builds (once, cached) the Host for this URL. ignore_www is always passed
/// as false here since host_ has already had "www." stripped, at
/// construction time, if requested.
const urlparser::Host& urlparser::Url::ensureHost() const noexcept {
    if (!host_cache_) {
        host_cache_.emplace(host_, false);
    }
    return *host_cache_;
}

const urlparser::Host& urlparser::Url::host() const noexcept { return ensureHost(); }
const std::string& urlparser::Url::suffix() const noexcept { return ensureHost().suffix(); }
const std::string& urlparser::Url::subdomain() const noexcept { return ensureHost().subdomain(); }
const std::string& urlparser::Url::domain() const noexcept { return ensureHost().domain(); }
std::string urlparser::Url::domainName() const noexcept { return ensureHost().domainName(); }

std::ostream& operator<<(std::ostream& os, const urlparser::QueryParams& v) {
    os << "[";
    for (const auto& e : v) {
        os << e << ", ";
    }
    os << (v.empty() ? "" : "\b\b") << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const urlparser::Url& dt) {
    os << dt.str();
    return os;
}
