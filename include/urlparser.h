/**
* @file urlparser.h
* @brief Defines classes for parsing and handling URLs and hosts.
*
* This file contains the declaration of the Url and Host classes, which provide
* functionality for parsing and managing URLs and hosts within them. The Url class
* allows extraction and manipulation of various components of a URL, while the Host
* class focuses on handling and extracting domain-related information from a host.
*
* Both classes are plain value types: no PIMPL, no heap allocation for the
* object itself. The only deferred/lazy work is the PSL-dependent domain/
* suffix split (Host) and, for Url, building its Host at all - both are
* computed once, on first access, and cached in `mutable` members.
*
* Example Usage:
* @code
*   // Creating a URL object and accessing its methods
*   urlparser::Url url("https://www.example.com/path/to/resource");
*   std::cout << "URL Protocol: " << url.protocol() << std::endl;
*   std::cout << "URL Domain: " << url.domain() << std::endl;
*   std::cout << "URL Suffix: " << url.suffix() << std::endl;
*   std::cout << "URL Query: " << url.query() << std::endl;
*
*   // Creating a Host object and calling its methods
*   urlparser::Host host("www.example.com");
*   std::cout << "Host Domain: " << host.domain() << std::endl;
*   std::cout << "Host Suffix: " << host.suffix() << std::endl;
*   std::cout << "Host Subdomain: " << host.subdomain() << std::endl;
* @endcode
*/

#ifndef URLPARSER_H
#define URLPARSER_H

// Single source of truth for the project version. Read by:
//   - cmake/DynamicVersion.cmake (sets PROJECT_VERSION for the CMake build)
//   - pyproject.toml's [tool.scikit-build.metadata.version] regex provider
//   - version.py (bump/tag helper)
#define URLPARSER_VERSION_MAJOR 2
#define URLPARSER_VERSION_MINOR 0
#define URLPARSER_VERSION_PATCH 0

#define URLPARSER_VERSION_ENCODE(maj,min,pat) (((maj)*10000)+((min)*100)+(pat))
#define URLPARSER_VERSION \
    URLPARSER_VERSION_ENCODE(URLPARSER_VERSION_MAJOR,URLPARSER_VERSION_MINOR,URLPARSER_VERSION_PATCH)

/** The version of liburlparser in hex: `(major << 16) | (minor << 8) | (patch)`. */
#define URLPARSER_VERSION_HEX \
    ((URLPARSER_VERSION_MAJOR << 16) | (URLPARSER_VERSION_MINOR << 8) | URLPARSER_VERSION_PATCH)

/* Internal helpers for URLPARSER_VERSION_STRING */
#define _URLPARSER_VERSION_XSTR(a,b,c) #a"."#b"."#c
#define _URLPARSER_VERSION_STR(a,b,c)  _URLPARSER_VERSION_XSTR(a,b,c)

/** The version string of liburlparser, e.g. "2.0.0". */
#define URLPARSER_VERSION_STRING \
    _URLPARSER_VERSION_STR(URLPARSER_VERSION_MAJOR,URLPARSER_VERSION_MINOR,URLPARSER_VERSION_PATCH)

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace urlparser {
constexpr bool DEFAULT_IGNORE_WWW = false;

/**
 * @class version
 * @brief Exposes the library version (from the URLPARSER_VERSION_* macros)
 *        at runtime, without needing to `#include` the macros directly.
 */
class version {
   public:
    static unsigned int major() noexcept { return URLPARSER_VERSION_MAJOR; }
    static unsigned int minor() noexcept { return URLPARSER_VERSION_MINOR; }
    static unsigned int patch() noexcept { return URLPARSER_VERSION_PATCH; }
    static unsigned int hex() noexcept { return URLPARSER_VERSION_HEX; }
    static std::string_view string() noexcept { return URLPARSER_VERSION_STRING; }

   private:
    version();
};

/**
 * @typedef QueryParams
 * @brief A vector of strings representing query parameters in a URL.
 */
using QueryParams = std::vector<std::string>;

/**
 * @brief Represents a Host part of a URL.
 *
 * The Host class encapsulates functionalities for handling the host component
 * of a URL. It provides methods to extract domain-specific details such as
 * suffix, domain, subdomain, and the full domain.
 *
 * A Host is a plain value type: constructing one is just a string copy. The
 * Public-Suffix-List lookup that domain()/subdomain()/suffix()/domainName()
 * need is deferred until one of them is first called, and cached from then on.
 */
class Host {
   public:
    /**
     * @brief Create a Host object from a URL string.
     * @param url The URL string to parse.
     * @param ignore_www Whether to ignore the "www" subdomain. Default is false.
     * @return A Host object representing the host part of the URL.
     */
    static Host fromUrl(const std::string& url,
                        const bool ignore_www = DEFAULT_IGNORE_WWW);

    /**
     * @brief Load the Public Suffix List from a file.
     * @param filepath Path to the PSL file.
     * @throws std::runtime_error If the file cannot be opened or parsed.
     */
    static void loadPslFromPath(const std::string& filepath);

    /**
     * @brief Load the Public Suffix List from a string.
     * @param filestr The PSL content as a string.
     * @throws std::runtime_error If the content cannot be parsed.
     */
    static void loadPslFromString(const std::string& filestr);

    /**
     * @brief Check if the Public Suffix List (PSL) is loaded.
     * @return true if the PSL is loaded, false otherwise.
     */
    static bool isPslLoaded() noexcept;

    /**
     * @brief Remove "www." from the beginning of a hostname.
     * @param host The hostname to process.
     * @return The hostname without "www." if it was present.
     */
    static std::string_view removeWWW(const std::string_view& host) noexcept;

   public:
    /**
     * @brief Construct a Host object from a hostname string.
     * @param host The hostname to parse.
     * @param ignore_www Whether to ignore the "www" subdomain. Default is false.
     */
    Host(std::string host, const bool ignore_www = DEFAULT_IGNORE_WWW);

    /**
     * @brief Default constructor for the Host class.
     * Creates an empty Host object.
     */
    Host() noexcept = default;

    /**
     * @brief Equality operator for comparing two Host objects.
     */
    bool operator==(const Host& other) const noexcept;

    /**
     * @brief Equality operator for comparing a Host object with a string.
     */
    bool operator==(const std::string& other) const noexcept;

    /** @brief The suffix of the host (e.g., "com" in "example.com"). */
    const std::string& suffix() const noexcept;
    /** @brief The domain of the host (e.g., "example" in "www.example.com"). */
    const std::string& domain() const noexcept;
    /** @brief The domain name, i.e. "<domain>.<suffix>". */
    std::string domainName() const noexcept;
    /** @brief The subdomain of the host (e.g., "www" in "www.example.com"). */
    const std::string& subdomain() const noexcept;
    /** @brief The full domain of the host (e.g., "example.com"). */
    const std::string& fulldomain() const noexcept;
    /** @brief The complete host string (same as fulldomain()). */
    const std::string& str() const noexcept;

   private:
    // Runs the PSL lookup + domain/subdomain/suffix split exactly once, the
    // first time any of those fields is actually requested.
    void ensureParsed() const noexcept;

    std::string host_;
    bool ignore_www_ = false;
    mutable bool parsed_ = false;
    mutable std::string domain_;
    mutable std::string subdomain_;
    mutable std::string suffix_;
    mutable std::string fulldomain_;
};

// Extra headroom (bytes) reserved beyond a URL's own length in Url's single
// internal storage buffer. Parsing itself never needs this - every field is
// a non-expanding substring of the input (case-folding only), so the sum of
// all fields' lengths can never exceed the URL's length; reserving exactly
// that is provably enough. The headroom only matters for mutating setters
// added *after* construction (not yet implemented): if a setter's new value
// is longer than the field's current span, it gets appended past the
// initially-parsed content, consuming this headroom before Url would need
// to grow (reallocate) its buffer. Override before #include "urlparser.h"
// for special cases (e.g. programs that rebuild many long fields via
// setters on the same Url instance).
#ifndef URLPARSER_ARENA_EXTRA_CAPACITY
#define URLPARSER_ARENA_EXTRA_CAPACITY 32
#endif

/**
 * @brief Represents a URL.
 *
 * The Url class provides functionalities for parsing and managing URLs. It
 * allows access to various components of a URL such as protocol, subdomain,
 * domain, suffix, query, fragment, userinfo, port, and parameters.
 *
 * Like Host, Url is a plain value type - parsing happens once in the
 * constructor; the only deferred work is building the (cheap) Host, which
 * only happens if `.host()`/`.domain()`/`.suffix()`/`.subdomain()` is
 * actually called.
 *
 * Internally, all string fields (scheme, userinfo, host, path, params,
 * query, fragment) are carved out of a single owned buffer, sized once at
 * construction (see URLPARSER_ARENA_EXTRA_CAPACITY) - instead of each field
 * being its own separately heap-allocated std::string. Since fields are
 * stored as (offset, length) pairs into that one buffer rather than raw
 * pointers, copying a Url is automatically correct (no dangling views) with
 * no custom copy/move logic needed.
 */
class Url {
   public:
    /**
     * @brief Check if the Public Suffix List (PSL) is loaded.
     */
    static bool isPslLoaded() noexcept;

    /**
     * @brief Extract the host from a given URL, without fully parsing it.
     */
    static std::string extractHost(std::string_view url) noexcept;

   public:
    /**
     * @brief Construct a Url object from a given URL string.
     * @param url The URL string to parse.
     * @param ignore_www Whether to ignore the "www" subdomain. Default is false.
     * @throws std::invalid_argument If the URL is malformed or cannot be parsed.
     */
    Url(std::string_view url, const bool ignore_www = DEFAULT_IGNORE_WWW);

    /**
     * @brief Default constructor for the Url class.
     * Creates an empty URL object.
     */
    Url() noexcept = default;

    /**
     * @brief Equality operator for comparing two Url objects.
     */
    bool operator==(const Url& other) const noexcept;

    /** @brief The complete URL as a string. */
    std::string str() const noexcept;
    /** @brief The protocol of the URL (e.g., "http", "https", "ftp"). */
    std::string_view protocol() const noexcept { return field(scheme_); }
    /** @brief The subdomain of the URL (e.g., "www" in "www.example.com"). */
    const std::string& subdomain() const noexcept;
    /** @brief The domain of the URL (e.g., "example" in "www.example.com"). */
    const std::string& domain() const noexcept;
    /** @brief The suffix of the URL (e.g., "com" in "www.example.com"). */
    const std::string& suffix() const noexcept;
    /** @brief The query string of the URL (e.g., "a=1&b=2"). */
    std::string_view query() const noexcept { return field(query_); }
    /** @brief The fragment of the URL (the part after '#'). */
    std::string_view fragment() const noexcept { return field(fragment_); }
    /** @brief The userinfo of the URL (e.g., "username:password"). */
    std::string_view userinfo() const noexcept { return field(userinfo_); }
    /** @brief The path, with '.'/'..' segments resolved. */
    std::string abspath() const noexcept;
    /** @brief The domain name, i.e. "<domain>.<suffix>". */
    std::string domainName() const noexcept;
    /** @brief The full domain of the URL (e.g., "example.com"). */
    std::string_view fulldomain() const noexcept { return field(host_); }
    /** @brief The port number of the URL, or 0 if not specified. */
    int port() const noexcept { return port_; }
    /** @brief The '&'-separated query parameters, split into a vector. */
    QueryParams params() const noexcept;
    /** @brief The Host object representing the host part of the URL. */
    const Host& host() const noexcept;

   private:
    // An (offset, length) pair into storage_ - not a raw pointer/view, so
    // that copying a Url (which deep-copies storage_) never needs to
    // "rebase" anything: the offsets stay correct as-is.
    struct Span {
        uint32_t pos = 0;
        uint32_t len = 0;
    };
    std::string_view field(Span s) const noexcept {
        return std::string_view(storage_).substr(s.pos, s.len);
    }

    const Host& ensureHost() const noexcept;

    std::string storage_;
    Span scheme_, userinfo_, host_, path_, params_, query_, fragment_;
    int port_ = 0;
    bool has_params_ = false;
    bool has_query_ = false;
    bool ignore_www_ = false;
    mutable std::optional<Host> host_cache_;
};
}  // namespace urlparser

/**
 * @brief Output stream operator for QueryParams.
 */
std::ostream& operator<<(std::ostream& os, const urlparser::QueryParams& dt);

/**
 * @brief Output stream operator for Url.
 */
std::ostream& operator<<(std::ostream& os, const urlparser::Url& dt);

/**
 * @brief Output stream operator for Host.
 */
std::ostream& operator<<(std::ostream& os, const urlparser::Host& dt);
#endif  // URLPARSER_H
