/**
* @file urlparser.h
* @brief Types for parsing URLs and classifying/handling their host part.
*
* RFC 3986 defines a URL's host grammar as:
*
*     host = IP-literal / IPv4address / reg-name
*
* i.e. a host is either an IP address or a domain name - never both, never
* something in between. This library's types mirror that split directly:
*
*   - `hostname` is a domain name ("www.example.com"): subdomain, domain,
*     and suffix, via a lazily-computed Public Suffix List lookup.
*   - `ipv4` / `ipv6` are IP addresses: no PSL, no subdomain/domain/suffix
*     (those concepts don't apply to "192.0.2.1"), but arithmetic
*     (++, --, +=, -=), comparisons, and integer conversions instead.
*   - `host = std::variant<hostname, ipv4, ipv6>` is whichever of the three
*     a given URL's host actually is - what `url::host()` returns, and what
*     `parse_host()`/`parse_host_from_url()` build directly.
*
* All are plain value types: no PIMPL, no heap allocation for the object
* itself beyond what a std::string/std::variant already needs. The only
* deferred work is the PSL-dependent domain/suffix split (hostname) and,
* for url, building/classifying its host at all - both are computed once,
* on first access, and cached in `mutable` members.
*
* Example usage:
* @code
*   urlparser::url u("https://www.example.com/path");
*   std::cout << "protocol: " << u.protocol() << "\n";
*   if (auto* h = std::get_if<urlparser::hostname>(&u.host())) {
*       std::cout << "domain: " << h->domain() << ", suffix: " << h->suffix() << "\n";
*   }
*
*   urlparser::url ipurl("http://192.168.1.1:8080/");
*   if (auto* v4 = std::get_if<urlparser::ipv4>(&ipurl.host())) {
*       std::cout << "IP as int: " << v4->to_uint32() << "\n";
*       std::cout << "next IP: " << (*v4 + 1).str() << "\n";
*   }
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

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
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
 * @brief A domain name used as a URL's host, e.g. "www.example.com".
 *
 * `hostname` encapsulates the domain-specific details of a host: suffix,
 * domain, subdomain, and the full (possibly www-stripped) domain. For an IP
 * address host, use `ipv4`/`ipv6` instead - see the file-level docs.
 *
 * A hostname is a plain value type: constructing one is just a string
 * copy. The Public-Suffix-List lookup that domain()/subdomain()/suffix()/
 * domain_name() need is deferred until one of them is first called, and
 * cached from then on.
 */
class hostname {
   public:
    /**
     * @brief Create a hostname from a URL string, without taking ownership.
     * Prefer this overload when the caller only has a borrowed view of the
     * URL (e.g. a buffer owned by something else, like a Python string) -
     * it never allocates anything beyond the final hostname string itself.
     * @param url The URL string to parse.
     * @param ignore_www Whether to ignore the "www" subdomain. Default is false.
     * @return A hostname object representing the host part of the URL.
     */
    static hostname from_url(std::string_view url,
                        const bool ignore_www = DEFAULT_IGNORE_WWW);

    /**
     * @brief Create a hostname from a URL string literal.
     * Disambiguation overload: without this, from_url("literal") would be
     * an ambiguous call between the string_view and string&& overloads (a
     * const char* converts equally well to either).
     */
    static hostname from_url(const char* url,
                        const bool ignore_www = DEFAULT_IGNORE_WWW);

    /**
     * @brief Create a hostname from a URL string.
     * @param url The URL string to parse.
     * @param ignore_www Whether to ignore the "www" subdomain. Default is false.
     * @return A hostname object representing the host part of the URL.
     */
    static hostname from_url(const std::string& url,
                        const bool ignore_www = DEFAULT_IGNORE_WWW);

    /**
     * @brief Create a hostname from a URL string, taking ownership of it.
     * Prefer this overload when you already have an owned/temporary
     * std::string you don't need afterward (e.g. a std::move'd variable):
     * it reuses that string's existing allocation to build the hostname in
     * place, instead of copying the host substring into a brand new one.
     * @param url The URL string to parse. Left in a valid but unspecified
     * state after the call.
     * @param ignore_www Whether to ignore the "www" subdomain. Default is false.
     * @return A hostname object representing the host part of the URL.
     */
    static hostname from_url(std::string&& url,
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
     * @brief Remove "www." from the beginning of a hostname string.
     * @param host The hostname text to process.
     * @return The text without a leading "www." if it was present.
     */
    static std::string_view remove_www(const std::string_view& host) noexcept;

   public:
    /**
     * @brief Construct a hostname object from a hostname string.
     * @param host The hostname to parse.
     * @param ignore_www Whether to ignore the "www" subdomain. Default is false.
     */
    hostname(std::string host, const bool ignore_www = DEFAULT_IGNORE_WWW);

    /**
     * @brief Default constructor for the hostname class.
     * Creates an empty hostname object.
     */
    hostname() noexcept = default;

    /**
     * @brief Equality operator for comparing two hostname objects.
     */
    bool operator==(const hostname& other) const noexcept;

    /**
     * @brief Equality operator for comparing a hostname object with a string.
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

/**
 * @brief An IPv4 address used as a URL's host, e.g. "192.0.2.1".
 *
 * A plain 32-bit value type (internally just a uint32_t), so every
 * operation - comparisons, ++/--/+=/-=, to_uint32() - is a single native
 * integer instruction; there's no PSL, no subdomain/domain/suffix, and no
 * family tag to branch on the way a combined IPv4-or-IPv6 type would need.
 *
 * Parsing/formatting use the platform's own inet_pton/inet_ntop (POSIX) or
 * InetPtonA/InetNtopA (Windows) rather than hand-written: even IPv4's text
 * format has edge cases (leading zeros, legacy octal/hex octets) better
 * left to a battle-tested implementation than re-derived here.
 */
class ipv4 {
   public:
    /** @brief Default-constructs 0.0.0.0. */
    ipv4() noexcept = default;

    /**
     * @brief Parse an IPv4 address from text.
     * @throws std::invalid_argument if `text` isn't a valid IPv4 address.
     */
    ipv4(std::string_view text);

    /** @brief Checks validity without throwing or constructing. */
    static bool is_valid(std::string_view text) noexcept;

    /** @brief Constructs from a 32-bit representation (host byte order), e.g. 0xC0A80101 = 192.168.1.1. */
    static ipv4 from_uint32(uint32_t address) noexcept;

    /**
     * @brief Extract the host from a URL and parse it as an IPv4 address.
     * @throws std::invalid_argument if the URL's host isn't a valid IPv4 address.
     */
    static ipv4 from_url(std::string_view url);

    /** @brief Canonical dotted-decimal text form, e.g. "192.0.2.1". */
    std::string str() const;

    /** @brief The address as a 32-bit integer (host byte order). Always valid - never throws. */
    uint32_t to_uint32() const noexcept { return value_; }

    /** @brief The address as 4 bytes, most significant first. */
    std::array<uint8_t, 4> bytes() const noexcept;

    ipv4& operator++() noexcept {
        ++value_;
        return *this;
    }
    ipv4 operator++(int) noexcept {
        ipv4 tmp = *this;
        ++value_;
        return tmp;
    }
    ipv4& operator--() noexcept {
        --value_;
        return *this;
    }
    ipv4 operator--(int) noexcept {
        ipv4 tmp = *this;
        --value_;
        return tmp;
    }
    /** @brief Steps the address by `delta`, wrapping around mod 2^32. */
    ipv4& operator+=(int64_t delta) noexcept {
        value_ += static_cast<uint32_t>(delta);
        return *this;
    }
    ipv4& operator-=(int64_t delta) noexcept {
        value_ -= static_cast<uint32_t>(delta);
        return *this;
    }

    friend ipv4 operator+(ipv4 lhs, int64_t delta) noexcept { return lhs += delta; }
    friend ipv4 operator-(ipv4 lhs, int64_t delta) noexcept { return lhs -= delta; }
    /** @brief Distance between two addresses (always fits in int64_t: the range is only 2^32). */
    friend int64_t operator-(const ipv4& a, const ipv4& b) noexcept {
        return static_cast<int64_t>(a.value_) - static_cast<int64_t>(b.value_);
    }

    friend bool operator==(const ipv4& a, const ipv4& b) noexcept { return a.value_ == b.value_; }
    friend bool operator!=(const ipv4& a, const ipv4& b) noexcept { return a.value_ != b.value_; }
    friend bool operator<(const ipv4& a, const ipv4& b) noexcept { return a.value_ < b.value_; }
    friend bool operator<=(const ipv4& a, const ipv4& b) noexcept { return a.value_ <= b.value_; }
    friend bool operator>(const ipv4& a, const ipv4& b) noexcept { return a.value_ > b.value_; }
    friend bool operator>=(const ipv4& a, const ipv4& b) noexcept { return a.value_ >= b.value_; }

   private:
    explicit ipv4(uint32_t v) noexcept : value_(v) {}
    uint32_t value_ = 0;
};

/**
 * @brief An IPv6 address used as a URL's host, e.g. "2001:db8::1".
 *
 * A plain 128-bit value type, stored as two 64-bit halves (high64()/
 * low64()) since C++ has no portable 128-bit integer - arithmetic is two
 * native 64-bit add-with-carry operations, not a byte-array loop.
 *
 * Parsing/formatting use the platform's own inet_pton/inet_ntop (POSIX) or
 * InetPtonA/InetNtopA (Windows): IPv6's text format (:: compression,
 * embedded IPv4 tails, zone IDs on some platforms) has enough edge cases
 * that a hand-written parser is far more likely to introduce bugs than to
 * avoid them.
 */
class ipv6 {
   public:
    /** @brief Default-constructs "::" (all zero). */
    ipv6() noexcept = default;

    /**
     * @brief Parse an IPv6 address from text.
     * Accepts a bracketed literal ("[::1]", as it appears in a URL's host
     * component) or a bare one ("::1").
     * @throws std::invalid_argument if `text` isn't a valid IPv6 address.
     */
    ipv6(std::string_view text);

    /** @brief Checks validity (bracketed or bare) without throwing or constructing. */
    static bool is_valid(std::string_view text) noexcept;

    /** @brief Constructs from the two 64-bit halves (high64, then low64), in host byte order. */
    static ipv6 from_uint64_pair(uint64_t high, uint64_t low) noexcept;

    /**
     * @brief Extract the host from a URL and parse it as an IPv6 address.
     * @throws std::invalid_argument if the URL's host isn't a valid IPv6 address.
     */
    static ipv6 from_url(std::string_view url);

    /** @brief Canonical (RFC 5952) text form, no brackets: "2001:db8::1", not "[2001:db8::1]". */
    std::string str() const;

    /** @brief The address as 16 bytes, most significant first. */
    std::array<uint8_t, 16> bytes() const noexcept;

    /** @brief The high 64 bits (first 8 bytes). */
    uint64_t high64() const noexcept { return hi_; }
    /** @brief The low 64 bits (last 8 bytes). */
    uint64_t low64() const noexcept { return lo_; }

    ipv6& operator++() noexcept {
        add_bits(1);
        return *this;
    }
    ipv6 operator++(int) noexcept {
        ipv6 tmp = *this;
        ++(*this);
        return tmp;
    }
    ipv6& operator--() noexcept {
        add_bits(~uint64_t{0});  // two's-complement bit pattern of -1
        return *this;
    }
    ipv6 operator--(int) noexcept {
        ipv6 tmp = *this;
        --(*this);
        return tmp;
    }
    /** @brief Steps the address by `delta`, wrapping around mod 2^128. */
    ipv6& operator+=(int64_t delta) noexcept {
        add_bits(static_cast<uint64_t>(delta));
        return *this;
    }
    ipv6& operator-=(int64_t delta) noexcept {
        // -delta's bit pattern, computed in unsigned arithmetic so this is
        // well-defined even when delta == INT64_MIN (no positive int64_t
        // counterpart, so negating it as a signed value would overflow).
        add_bits(~static_cast<uint64_t>(delta) + 1);
        return *this;
    }

    friend ipv6 operator+(ipv6 lhs, int64_t delta) noexcept { return lhs += delta; }
    friend ipv6 operator-(ipv6 lhs, int64_t delta) noexcept { return lhs -= delta; }

    friend bool operator==(const ipv6& a, const ipv6& b) noexcept {
        return a.hi_ == b.hi_ && a.lo_ == b.lo_;
    }
    friend bool operator!=(const ipv6& a, const ipv6& b) noexcept { return !(a == b); }
    friend bool operator<(const ipv6& a, const ipv6& b) noexcept {
        return a.hi_ != b.hi_ ? a.hi_ < b.hi_ : a.lo_ < b.lo_;
    }
    friend bool operator<=(const ipv6& a, const ipv6& b) noexcept { return !(b < a); }
    friend bool operator>(const ipv6& a, const ipv6& b) noexcept { return b < a; }
    friend bool operator>=(const ipv6& a, const ipv6& b) noexcept { return !(a < b); }

   private:
    ipv6(uint64_t hi, uint64_t lo) noexcept : hi_(hi), lo_(lo) {}

    // Adds the 128-bit sign-extension of the two's-complement 64-bit
    // pattern `low64_bits` to (hi_, lo_), wrapping mod 2^128.
    void add_bits(uint64_t low64_bits) noexcept;

    uint64_t hi_ = 0;
    uint64_t lo_ = 0;
};

/**
 * @brief `hostname`, `ipv4`, or `ipv6`: whatever a URL's host actually is
 * (RFC 3986's `host` grammar), wrapped in a uniform API that matches
 * hostname/ipv4/ipv6/url themselves: a constructor that just parses, plus
 * from_url() and str().
 *
 * Internally this is a std::variant<hostname, ipv4, ipv6> held by value
 * (composition, not inheritance - deriving public classes from a standard
 * container is a well-known footgun: no virtual destructor, and an
 * interface you don't fully control). is_hostname()/is_ipv4()/is_ipv6(),
 * get_hostname()/get_ipv4()/get_ipv6() (throwing, like std::get<T>()), and
 * try_hostname()/try_ipv4()/try_ipv6() (non-throwing, like
 * std::get_if<T>()) cover the common cases without needing std::variant at
 * all; variant() gives direct access to the underlying std::variant for
 * std::visit or structured, exhaustive handling.
 */
class host {
   public:
    using variant_type = std::variant<hostname, ipv4, ipv6>;

    /** @brief Default-constructs an empty hostname. */
    host() noexcept = default;

    /** @brief Wraps an already-classified hostname/ipv4/ipv6 as a host. */
    static host wrap(hostname h) noexcept {
        host result;
        result.value_ = std::move(h);
        return result;
    }
    static host wrap(ipv4 v) noexcept {
        host result;
        result.value_ = v;
        return result;
    }
    static host wrap(ipv6 v) noexcept {
        host result;
        result.value_ = v;
        return result;
    }

    /**
     * @brief Classify and parse an already-isolated host string (no
     * scheme, no path - e.g. what url::host_text() gives you) as whichever
     * of hostname/ipv4/ipv6 it actually is. Never throws: anything that
     * isn't a valid IPv4/IPv6 address becomes a hostname.
     * @param host_text The host text, e.g. "example.com", "192.0.2.1", or
     * "[2001:db8::1]" (a bracketed IPv6 literal, as it appears in a URL).
     * @param ignore_www Only meaningful if the result is a hostname; ignored for IPs.
     */
    host(std::string_view host_text, bool ignore_www = DEFAULT_IGNORE_WWW) noexcept;

    /**
     * @brief Classify a host string literal.
     * Disambiguation overload: without this, host("literal") would be
     * ambiguous between the string_view and string&& overloads.
     */
    host(const char* host_text, bool ignore_www = DEFAULT_IGNORE_WWW) noexcept;

    /**
     * @brief Classify an already-isolated host string, taking ownership of
     * it. When the text turns out to be a domain name, this reuses its
     * existing allocation to build the resulting hostname instead of
     * copying into a new one.
     * @param host_text The host text. Left in a valid but unspecified state.
     */
    host(std::string&& host_text, bool ignore_www = DEFAULT_IGNORE_WWW) noexcept;

    /**
     * @brief Extract the host from a full URL and classify/parse it, in
     * one step.
     */
    static host from_url(std::string_view url, bool ignore_www = DEFAULT_IGNORE_WWW) noexcept;

    /**
     * @brief Classify and parse the host of a URL string literal.
     * Disambiguation overload, same reason as host(const char*, bool).
     */
    static host from_url(const char* url, bool ignore_www = DEFAULT_IGNORE_WWW) noexcept;

    /**
     * @brief Extract the host from a full URL and classify/parse it,
     * taking ownership of the URL string - reuses its allocation
     * end-to-end (extract, then classify) when the result is a hostname.
     * @param url The URL string. Left in a valid but unspecified state.
     */
    static host from_url(std::string&& url, bool ignore_www = DEFAULT_IGNORE_WWW) noexcept;

    bool is_hostname() const noexcept { return std::holds_alternative<hostname>(value_); }
    bool is_ipv4() const noexcept { return std::holds_alternative<ipv4>(value_); }
    bool is_ipv6() const noexcept { return std::holds_alternative<ipv6>(value_); }
    /** @brief True for either IP family - the opposite of is_hostname(). */
    bool is_ip() const noexcept { return !is_hostname(); }

    /** @throws std::bad_variant_access if this isn't a hostname. */
    const hostname& get_hostname() const { return std::get<hostname>(value_); }
    /** @throws std::bad_variant_access if this isn't an ipv4. */
    const ipv4& get_ipv4() const { return std::get<ipv4>(value_); }
    /** @throws std::bad_variant_access if this isn't an ipv6. */
    const ipv6& get_ipv6() const { return std::get<ipv6>(value_); }

    /** @brief nullptr if this isn't a hostname. */
    const hostname* try_hostname() const noexcept { return std::get_if<hostname>(&value_); }
    /** @brief nullptr if this isn't an ipv4. */
    const ipv4* try_ipv4() const noexcept { return std::get_if<ipv4>(&value_); }
    /** @brief nullptr if this isn't an ipv6. */
    const ipv6* try_ipv6() const noexcept { return std::get_if<ipv6>(&value_); }

    /** @brief The text form, whichever of hostname/ipv4/ipv6 this is. */
    std::string str() const;

    /** @brief Direct access to the underlying variant - for std::visit, std::get, etc. */
    const variant_type& variant() const noexcept { return value_; }

    friend bool operator==(const host& a, const host& b) noexcept { return a.value_ == b.value_; }
    friend bool operator!=(const host& a, const host& b) noexcept { return !(a == b); }

   private:
    variant_type value_;
};

/**
 * @brief Represents a URL.
 *
 * The url class provides functionalities for parsing and managing URLs. It
 * allows access to various components of a URL such as protocol, host,
 * query, fragment, userinfo, port, and parameters.
 *
 * Like hostname, url is a plain value type - parsing happens once in the
 * constructor; the only deferred work is classifying/building the (cheap)
 * host, which only happens if `.host()` is actually called.
 *
 * Internally, all string fields (scheme, userinfo, host, path, params,
 * query, fragment) are carved out of a single owned buffer, sized exactly
 * once at construction - instead of each field being its own separately
 * heap-allocated std::string. Since fields are stored as (offset, length)
 * pairs into that one buffer rather than raw pointers, copying a url is
 * automatically correct (no dangling views) with no custom copy/move logic
 * needed.
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
    /** @brief The query string of the URL (e.g., "a=1&b=2"). */
    std::string_view query() const noexcept { return field(query_); }
    /** @brief The fragment of the URL (the part after '#'). */
    std::string_view fragment() const noexcept { return field(fragment_); }
    /** @brief The userinfo of the URL (e.g., "username:password"). */
    std::string_view userinfo() const noexcept { return field(userinfo_); }
    /** @brief The path, with '.'/'..' segments resolved. */
    std::string abspath() const noexcept;
    /** @brief The raw host text (e.g., "example.com", "192.0.2.1", or "[::1]"), before hostname/ip classification. */
    std::string_view host_text() const noexcept { return field(host_); }
    /** @brief The port number of the URL, or 0 if not specified. */
    int port() const noexcept { return port_; }
    /** @brief The '&'-separated query parameters, split into a vector. */
    QueryParams params() const noexcept;
    /**
     * @brief The host part of the URL, classified as a hostname, ipv4, or ipv6.
     * Use std::get_if<hostname>/<ipv4>/<ipv6>(&url.host()) or std::visit to
     * access it - see the file-level docs for an example.
     */
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
 * @brief Output stream operator for hostname.
 */
std::ostream& operator<<(std::ostream& os, const urlparser::hostname& dt);

/**
 * @brief Output stream operator for ipv4.
 */
std::ostream& operator<<(std::ostream& os, const urlparser::ipv4& dt);

/**
 * @brief Output stream operator for ipv6.
 */
std::ostream& operator<<(std::ostream& os, const urlparser::ipv6& dt);

/**
 * @brief Output stream operator for the host variant (hostname/ipv4/ipv6).
 */
std::ostream& operator<<(std::ostream& os, const urlparser::host& dt);
#endif  // URLPARSER_H
