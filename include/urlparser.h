/**
* @file urlparser.h
* @brief Defines classes for parsing and handling URLs and hosts.
*
* This file contains the declaration of the url and host classes, which provide
* functionality for parsing and managing URLs and hosts within them. The url class
* allows extraction and manipulation of various components of a URL, while the host
* class focuses on handling and extracting domain-related information from a host.
*
* Both classes are plain value types: no PIMPL, no heap allocation for the
* object itself. The only deferred/lazy work is the PSL-dependent domain/
* suffix split (host) and, for url, building its host at all - both are
* computed once, on first access, and cached in `mutable` members.
*
* Example Usage:
* @code
*   // Creating a URL object and accessing its methods
*   urlparser::url url("https://www.example.com/path/to/resource");
*   std::cout << "URL Protocol: " << url.protocol() << std::endl;
*   std::cout << "URL Domain: " << url.domain() << std::endl;
*   std::cout << "URL Suffix: " << url.suffix() << std::endl;
*   std::cout << "URL Query: " << url.query() << std::endl;
*
*   // Creating a host object and calling its methods
*   urlparser::host host("www.example.com");
*   std::cout << "host Domain: " << host.domain() << std::endl;
*   std::cout << "host Suffix: " << host.suffix() << std::endl;
*   std::cout << "host Subdomain: " << host.subdomain() << std::endl;
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
 * @brief Represents a host part of a URL.
 *
 * The host class encapsulates functionalities for handling the host component
 * of a URL. It provides methods to extract domain-specific details such as
 * suffix, domain, subdomain, and the full domain.
 *
 * A host is a plain value type: constructing one is just a string copy. The
 * Public-Suffix-List lookup that domain()/subdomain()/suffix()/domain_name()
 * need is deferred until one of them is first called, and cached from then on.
 */
class host {
   public:
    /**
     * @brief Create a host object from a URL string, without taking ownership.
     * Prefer this overload when the caller only has a borrowed view of the
     * URL (e.g. a buffer owned by something else, like a Python string) -
     * it never allocates anything beyond the final host string itself.
     * @param url The URL string to parse.
     * @param ignore_www Whether to ignore the "www" subdomain. Default is false.
     * @return A host object representing the host part of the URL.
     */
    static host from_url(std::string_view url,
                        const bool ignore_www = DEFAULT_IGNORE_WWW);

    /**
     * @brief Create a host object from a URL string literal.
     * Disambiguation overload: without this, from_url("literal") would be
     * an ambiguous call between the string_view and string&& overloads (a
     * const char* converts equally well to either).
     */
    static host from_url(const char* url,
                        const bool ignore_www = DEFAULT_IGNORE_WWW);

    /**
     * @brief Create a host object from a URL string.
     * @param url The URL string to parse.
     * @param ignore_www Whether to ignore the "www" subdomain. Default is false.
     * @return A host object representing the host part of the URL.
     */
    static host from_url(const std::string& url,
                        const bool ignore_www = DEFAULT_IGNORE_WWW);

    /**
     * @brief Create a host object from a URL string, taking ownership of it.
     * Prefer this overload when you already have an owned/temporary
     * std::string you don't need afterward (e.g. a std::move'd variable):
     * it reuses that string's existing allocation to build the host in
     * place, instead of copying the host substring into a brand new one.
     * @param url The URL string to parse. Left in a valid but unspecified
     * state after the call.
     * @param ignore_www Whether to ignore the "www" subdomain. Default is false.
     * @return A host object representing the host part of the URL.
     */
    static host from_url(std::string&& url,
                        const bool ignore_www = DEFAULT_IGNORE_WWW);

    /**
     * @brief Load the Public Suffix List from a file.
     * @param filepath Path to the PSL file.
     * @throws std::runtime_error If the file cannot be opened or parsed.
     */
    static void load_psl_from_path(const std::string& filepath);

    /**
     * @brief Load the Public Suffix List from a string.
     * @param filestr The PSL content as a string.
     * @throws std::runtime_error If the content cannot be parsed.
     */
    static void load_psl_from_string(const std::string& filestr);

    /**
     * @brief Check if the Public Suffix List (PSL) is loaded.
     * @return true if the PSL is loaded, false otherwise.
     */
    static bool is_psl_loaded() noexcept;

    /**
     * @brief Remove "www." from the beginning of a hostname.
     * @param host The hostname to process.
     * @return The hostname without "www." if it was present.
     */
    static std::string_view remove_www(const std::string_view& host) noexcept;

   public:
    /**
     * @brief Construct a host object from a hostname string.
     * @param host The hostname to parse.
     * @param ignore_www Whether to ignore the "www" subdomain. Default is false.
     */
    host(std::string host, const bool ignore_www = DEFAULT_IGNORE_WWW);

    /**
     * @brief Default constructor for the host class.
     * Creates an empty host object.
     */
    host() noexcept = default;

    /**
     * @brief Equality operator for comparing two host objects.
     */
    bool operator==(const host& other) const noexcept;

    /**
     * @brief Equality operator for comparing a host object with a string.
     */
    bool operator==(const std::string& other) const noexcept;

    /** @brief The suffix of the host (e.g., "com" in "example.com"). */
    const std::string& suffix() const noexcept;
    /** @brief The domain of the host (e.g., "example" in "www.example.com"). */
    const std::string& domain() const noexcept;
    /** @brief The domain name, i.e. "<domain>.<suffix>". */
    std::string domain_name() const noexcept;
    /** @brief The subdomain of the host (e.g., "www" in "www.example.com"). */
    const std::string& subdomain() const noexcept;
    /** @brief The full domain of the host (e.g., "example.com"). */
    const std::string& full_domain() const noexcept;
    /** @brief The complete host string (same as full_domain()). */
    const std::string& str() const noexcept;

   private:
    // Runs the PSL lookup + domain/subdomain/suffix split exactly once, the
    // first time any of those fields is actually requested.
    void ensure_parsed() const noexcept;

    std::string host_;
    bool ignore_www_ = false;
    mutable bool parsed_ = false;
    mutable std::string domain_;
    mutable std::string subdomain_;
    mutable std::string suffix_;
    mutable std::string fulldomain_;
};

// Extra headroom (bytes) reserved beyond a URL's own length in url's single
// internal storage buffer. Parsing itself never needs this - every field is
// a non-expanding substring of the input (case-folding only), so the sum of
// all fields' lengths can never exceed the URL's length; reserving exactly
// that is provably enough. The headroom only matters for mutating setters
// added *after* construction (not yet implemented): if a setter's new value
// is longer than the field's current span, it gets appended past the
// initially-parsed content, consuming this headroom before url would need
// to grow (reallocate) its buffer. Override before #include "urlparser.h"
// for special cases (e.g. programs that rebuild many long fields via
// setters on the same url instance).
#ifndef URLPARSER_ARENA_EXTRA_CAPACITY
#define URLPARSER_ARENA_EXTRA_CAPACITY 32
#endif

/**
 * @brief Represents a URL.
 *
 * The url class provides functionalities for parsing and managing URLs. It
 * allows access to various components of a URL such as protocol, subdomain,
 * domain, suffix, query, fragment, userinfo, port, and parameters.
 *
 * Like host, url is a plain value type - parsing happens once in the
 * constructor; the only deferred work is building the (cheap) host, which
 * only happens if `.host()`/`.domain()`/`.suffix()`/`.subdomain()` is
 * actually called.
 *
 * Internally, all string fields (scheme, userinfo, host, path, params,
 * query, fragment) are carved out of a single owned buffer, sized once at
 * construction (see URLPARSER_ARENA_EXTRA_CAPACITY) - instead of each field
 * being its own separately heap-allocated std::string. Since fields are
 * stored as (offset, length) pairs into that one buffer rather than raw
 * pointers, copying a url is automatically correct (no dangling views) with
 * no custom copy/move logic needed.
 */
class url {
   public:
    /**
     * @brief Check if the Public Suffix List (PSL) is loaded.
     */
    static bool is_psl_loaded() noexcept;

    /**
     * @brief Extract the host from a given URL, without fully parsing it.
     */
    static std::string extract_host(std::string_view url) noexcept;

    /**
     * @brief Extract the host from a given URL literal.
     * Disambiguation overload: without this, extract_host("literal") would
     * be an ambiguous call between the string_view and string&& overloads
     * (a const char* converts equally well to either). Routes to the
     * string_view overload, since a literal has no heap buffer to reuse.
     */
    static std::string extract_host(const char* url) noexcept;

    /**
     * @brief Extract the host from a given URL, taking ownership of it.
     * Reuses url's own buffer (via erase()) to build the result instead of
     * allocating a new string, so prefer this overload when you have an
     * owned/temporary std::string you don't need afterward.
     * @param url The URL string, left in a valid but unspecified state.
     */
    static std::string extract_host(std::string&& url) noexcept;

   public:
    /**
     * @brief Construct a url object from a given URL string.
     * @param url The URL string to parse.
     * @param ignore_www Whether to ignore the "www" subdomain. Default is false.
     * @throws std::invalid_argument If the URL is malformed or cannot be parsed.
     */
    url(std::string_view url, const bool ignore_www = DEFAULT_IGNORE_WWW);

    /**
     * @brief Default constructor for the url class.
     * Creates an empty URL object.
     */
    url() noexcept = default;

    /**
     * @brief Equality operator for comparing two url objects.
     */
    bool operator==(const url& other) const noexcept;

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
    std::string domain_name() const noexcept;
    /** @brief The full domain of the URL (e.g., "example.com"). */
    std::string_view full_domain() const noexcept { return field(host_); }
    /** @brief The port number of the URL, or 0 if not specified. */
    int port() const noexcept { return port_; }
    /** @brief The '&'-separated query parameters, split into a vector. */
    QueryParams params() const noexcept;
    /** @brief The host object representing the host part of the URL. */
    const urlparser::host& host() const noexcept;

   private:
    // An (offset, length) pair into storage_ - not a raw pointer/view, so
    // that copying a url (which deep-copies storage_) never needs to
    // "rebase" anything: the offsets stay correct as-is.
    struct Span {
        uint32_t pos = 0;
        uint32_t len = 0;
    };
    std::string_view field(Span s) const noexcept {
        return std::string_view(storage_).substr(s.pos, s.len);
    }

    const urlparser::host& ensure_host() const noexcept;

    std::string storage_;
    Span scheme_, userinfo_, host_, path_, params_, query_, fragment_;
    int port_ = 0;
    bool has_params_ = false;
    bool has_query_ = false;
    bool ignore_www_ = false;
    mutable std::optional<urlparser::host> host_cache_;
};
}  // namespace urlparser

/**
 * @brief Output stream operator for QueryParams.
 */
std::ostream& operator<<(std::ostream& os, const urlparser::QueryParams& dt);

/**
 * @brief Output stream operator for url.
 */
std::ostream& operator<<(std::ostream& os, const urlparser::url& dt);

/**
 * @brief Output stream operator for host.
 */
std::ostream& operator<<(std::ostream& os, const urlparser::host& dt);
#endif  // URLPARSER_H
