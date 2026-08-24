//
// Host: Public-Suffix-List matching (suffix/domain/subdomain split).
//
// This is the single file for everything Host/PSL-related (previously split
// across psl.h + psl.cpp + urlparser_host.cpp, with a PIMPL Host::Impl
// wrapping a separately-loaded PSL). Host is now a flat value type declared
// directly in include/urlparser.h; PSL (the one piece that genuinely needs a
// std::unordered_map) lives entirely here, in urlparser::detail, and never
// appears in any header at all - so there was never really a need to hide it
// behind a PIMPL on Host's instances, only to keep it out of the header.
//
#include "urlparser.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "public_suffix_list_dat.h"

namespace urlparser::detail {

// See urlparser.cpp for the rationale (avoids the locale-table indirection
// that ::tolower pays on every call - this runs on every character of every
// hostname looked up against the PSL).
inline char ascii_tolower(char c) noexcept {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
}

/**
 * Finds the TLD (public suffix) of a hostname according to a Public Suffix
 * List. Internal to this file - Host is the only thing that uses it.
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

void urlparser::Host::loadPslFromPath(const std::string& filepath) {
    urlparser::detail::psl() = urlparser::detail::PSL::fromPath(filepath);
}

void urlparser::Host::loadPslFromString(const std::string& filestr) {
    urlparser::detail::psl() = urlparser::detail::PSL::fromString(filestr);
}

bool urlparser::Host::isPslLoaded() noexcept {
    return urlparser::detail::psl().numLevels() > 0;
}

std::string_view urlparser::Host::removeWWW(const std::string_view& host) noexcept {
    if (host.compare(0, 4, "www.") != 0) {
        return host;
    }
    return host.substr(4);
}

urlparser::Host::Host(const std::string& host, const bool ignore_www)
    : host_(host), ignore_www_(ignore_www) {}

urlparser::Host urlparser::Host::fromUrl(const std::string& url, const bool ignore_www) {
    return urlparser::Host(urlparser::Url::extractHost(url), ignore_www);
}

void urlparser::Host::ensureParsed() const noexcept {
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

const std::string& urlparser::Host::suffix() const noexcept {
    ensureParsed();
    return suffix_;
}

const std::string& urlparser::Host::subdomain() const noexcept {
    ensureParsed();
    return subdomain_;
}

const std::string& urlparser::Host::domain() const noexcept {
    ensureParsed();
    return domain_;
}

// Fast path: with ignore_www == false, fulldomain is always exactly the
// original host string (the PSL-dependent split above never touches
// fulldomain_ in that case), so this skips the PSL lookup entirely for what
// is, by far, the most common call pattern.
const std::string& urlparser::Host::fulldomain() const noexcept {
    if (!ignore_www_) return host_;
    ensureParsed();
    return fulldomain_;
}

const std::string& urlparser::Host::str() const noexcept { return fulldomain(); }

std::string urlparser::Host::domainName() const noexcept {
    ensureParsed();
    return domain_ + "." + suffix_;
}

bool urlparser::Host::operator==(const urlparser::Host& other) const noexcept {
    return fulldomain() == other.fulldomain();
}

bool urlparser::Host::operator==(const std::string& other) const noexcept {
    return fulldomain() == other;
}

std::ostream& operator<<(std::ostream& os, const urlparser::Host& dt) {
    os << dt.str();
    return os;
}
