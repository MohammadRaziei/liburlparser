/**
* @file urlparser.h
* @brief Defines classes for parsing and handling URLs and hosts.
*
* This file contains the declaration of the Url and Host classes, which provide
* functionality for parsing and managing URLs and hosts within them. The Url class
* allows extraction and manipulation of various components of a URL, while the Host
* class focuses on handling and extracting domain-related information from a host.
*
* The Url class encapsulates a URL and provides methods to access its components
* such as protocol, subdomain, domain, suffix, query, fragment, userinfo, port,
* and parameters. Additionally, it offers methods for constructing a URL string and
* obtaining specific parts of the URL.
*
* The Host class represents a host part of a URL and offers functionalities for
* extracting domain-related details such as suffix, domain, subdomain, and the full
* domain. It also supports the removal of 'www' from the host and provides methods
* to obtain domain-specific information.
*
* Example Usage:
* @code
*   // Creating a URL object and accessing its methods
*   TLD::Url url("https://www.example.com/path/to/resource");
*   std::cout << "URL Protocol: " << url.protocol() << std::endl;
*   std::cout << "URL Domain: " << url.domain() << std::endl;
*   std::cout << "URL Suffix: " << url.suffix() << std::endl;
*   std::cout << "URL Query: " << url.query() << std::endl;
*
*   // Creating a Host object and calling its methods
*   TLD::Host host("www.example.com");
*   std::cout << "Host Domain: " << host.domain() << std::endl;
*   std::cout << "Host Suffix: " << host.suffix() << std::endl;
*   std::cout << "Host Subdomain: " << host.subdomain() << std::endl;
* @endcode
*/

#ifndef TLD_URLPARSER_H
#define TLD_URLPARSER_H

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifndef PUBLIC_SUFFIX_LIST_DAT
#define PUBLIC_SUFFIX_LIST_DAT "public_suffix_list.dat"
#endif

namespace TLD {
constexpr bool DEFAULT_IGNORE_WWW = false;

/**
 * @typedef QueryParams
 * @brief A vector of strings representing query parameters in a URL.
 */
using QueryParams = std::vector<std::string>;

class Host;

/**
 * @brief Represents a URL.
 *
 * The Url class provides functionalities for parsing and managing URLs. It
 * allows access to various components of a URL such as protocol, subdomain,
 * domain, suffix, query, fragment, userinfo, port, and parameters.
 */
class Url {
   public:
    /**
     * @brief Check if the Public Suffix List (PSL) is loaded.
     * @return true if the PSL is loaded, false otherwise.
     */
    static bool isPslLoaded() noexcept;
    
    /**
     * @brief Extract the host from a given URL.
     * @param url The URL from which to extract the host.
     * @return The extracted host as a string.
     */
    static std::string extractHost(const std::string& url) noexcept;

   public:
    /**
     * @brief Construct a Url object from a given URL string.
     * @param url The URL string to parse.
     * @param ignore_www Whether to ignore the "www" subdomain. Default is false.
     * @throws std::invalid_argument If the URL is malformed or cannot be parsed.
     */
    Url(const std::string& url, const bool ignore_www = DEFAULT_IGNORE_WWW);
    
    /**
     * @brief Default constructor for the Url class.
     * Creates an empty URL object.
     */
    Url() noexcept = default;

    /**
     * @brief Equality operator for comparing two Url objects.
     * @param other The Url object to compare with.
     * @return true if the URLs are equal, false otherwise.
     */
    bool operator==(const Url& other) const;

    /**
     * @brief Get the complete URL as a string.
     * @return The complete URL string.
     */
    std::string str() const noexcept;
    
    /**
     * @brief Get the protocol of the URL.
     * @return The protocol of the URL (e.g., "http", "https", "ftp").
     */
    const std::string& protocol() const noexcept;
    
    /**
     * @brief Get the subdomain of the URL.
     * @return The subdomain of the URL (e.g., "www" in "www.example.com").
     */
    const std::string& subdomain() const noexcept;
    
    /**
     * @brief Get the domain of the URL.
     * @return The domain of the URL (e.g., "example" in "www.example.com").
     */
    const std::string& domain() const noexcept;
    
    /**
     * @brief Get the suffix of the URL.
     * @return The suffix of the URL (e.g., "com" in "www.example.com").
     */
    const std::string& suffix() const noexcept;
    
    /**
     * @brief Get the query part of the URL.
     * @return The query string of the URL (e.g., "param1=value1&param2=value2").
     */
    const std::string& query() const noexcept;
    
    /**
     * @brief Get the fragment part of the URL.
     * @return The fragment of the URL (the part after #).
     */
    const std::string& fragment() const noexcept;
    
    /**
     * @brief Get the userinfo part of the URL.
     * @return The userinfo of the URL (e.g., "username:password").
     */
    const std::string& userinfo() const noexcept;
    
    /**
     * @brief Get the absolute path of the URL.
     * @return The absolute path of the URL (the part after the host and before the query).
     */
    std::string abspath() const noexcept;
    
    /**
     * @brief Get the domain name of the URL.
     * @return The domain name, which is the same as domain().
     */
    std::string domainName() const noexcept;
    
    /**
     * @brief Get the full domain of the URL.
     * @return The full domain of the URL (e.g., "example.com").
     */
    const std::string& fulldomain() const noexcept;
    
    /**
     * @brief Get the port of the URL.
     * @return The port number of the URL, or -1 if not specified.
     */
    const int port() const noexcept;
    
    /**
     * @brief Get the parameters of the URL.
     * @return A vector of strings containing the query parameters.
     */
    QueryParams params() const noexcept;
    
    /**
     * @brief Get the host object of the URL.
     * @return A Host object representing the host part of the URL.
     */
    const Host& host() const;

   private:
    class Impl;
    std::shared_ptr<Impl> impl; // since all methods are constants
};

/**
 * @brief Represents a Host part of a URL.
 *
 * The Host class encapsulates functionalities for handling the host component
 * of a URL. It provides methods to extract domain-specific details such as
 * suffix, domain, subdomain, and the full domain.
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
     * @throws std::invalid_argument If the hostname is malformed or cannot be parsed.
     */
    Host(const std::string& host, const bool ignore_www = DEFAULT_IGNORE_WWW);
    
    /**
     * @brief Default constructor for the Host class.
     * Creates an empty Host object.
     */
    Host() = default;

    /**
     * @brief Equality operator for comparing two Host objects.
     * @param other The Host object to compare with.
     * @return true if the hosts are equal, false otherwise.
     */
    bool operator==(const Host& other) const;
    
    /**
     * @brief Equality operator for comparing a Host object with a string.
     * @param other The string to compare with.
     * @return true if the host string is equal to the given string, false otherwise.
     */
    bool operator==(const std::string& other) const;

    /**
     * @brief Get the suffix of the host.
     * @return The suffix of the host (e.g., "com" in "example.com").
     */
    const std::string& suffix() const noexcept;
    
    /**
     * @brief Get the domain of the host.
     * @return The domain of the host (e.g., "example" in "www.example.com").
     */
    const std::string& domain() const noexcept;
    
    /**
     * @brief Get the domain name of the host.
     * @return The domain name, which is the same as domain().
     */
    std::string domainName() const noexcept;
    
    /**
     * @brief Get the subdomain of the host.
     * @return The subdomain of the host (e.g., "www" in "www.example.com").
     */
    const std::string& subdomain() const noexcept;
    
    /**
     * @brief Get the full domain of the host.
     * @return The full domain of the host (e.g., "example.com").
     */
    const std::string& fulldomain() const noexcept;
    
    /**
     * @brief Get the complete host as a string.
     * @return The complete host string.
     */
    const std::string& str() const noexcept;

   private:
    class Impl;
    std::shared_ptr<Impl> impl; // since all methods are constants
};
}  // namespace TLD

/**
 * @brief Output stream operator for QueryParams.
 * @param os The output stream.
 * @param dt The QueryParams to output.
 * @return The output stream.
 */
std::ostream& operator<<(std::ostream& os, const TLD::QueryParams& dt);

/**
 * @brief Output stream operator for Url.
 * @param os The output stream.
 * @param dt The Url object to output.
 * @return The output stream.
 */
std::ostream& operator<<(std::ostream& os, const TLD::Url& dt);

/**
 * @brief Output stream operator for Host.
 * @param os The output stream.
 * @param dt The Host object to output.
 * @return The output stream.
 */
std::ostream& operator<<(std::ostream& os, const TLD::Host& dt);
#endif  // TLD_URLPARSER_H
