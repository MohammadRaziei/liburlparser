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
     * @return The extracted host.
     */
    static std::string extractHost(const std::string& url) noexcept;

   public:
    /**
     * @brief Construct a Url object from a given URL string.
     * @param url The URL string.
     * @param ignore_www Whether to ignore the "www" subdomain.
     */
    Url(const std::string& url, const bool ignore_www = DEFAULT_IGNORE_WWW);
    /**
     * @brief Default constructor for the Url class.
     */
    Url() noexcept = default;

    bool operator==(const Url&) const;

    std::string str() const noexcept;
    /**
     * @brief Get the protocol of the URL.
     * @return str Protocol of the URL.
     */
    const std::string& protocol() const noexcept;
    /**
     * @brief Get the Subdomain of the URL.
     * @return str Subdomain of the URL.
     */
    const std::string& subdomain() const noexcept;
    /**
     * @brief Get the Domain of the URL.
     * @return str Domain of the URL.
     */
    const std::string& domain() const noexcept;
    /**
     * @brief Get the Suffix of the URL.
     * @return str Suffix of the URL.
     */
    const std::string& suffix() const noexcept;
    /**
     * @brief Get the Query of the URL.
     * @return str Query of the URL.
     */
    const std::string& query() const noexcept;
    /**
     * @brief Get the Fragment of the URL.
     * @return str Fragment of the URL.
     */
    const std::string& fragment() const noexcept;
    /**
     * @brief Get the Userinfo of the URL.
     * @return str Userinfo of the URL.
     */
    const std::string& userinfo() const noexcept;
    /**
     * @brief Get the abspath of the URL.
     * @return str abspath of the URL.
     */
    std::string abspath() const noexcept;
    /**
     * @brief Get the domain name of the URL.
     * it is the same as domain() + "." + subdomain().
     * @return str port of the URL.
     */
    std::string domainName() const noexcept;
    /**
     * @brief Get the full domain of the URL.
     * @return str full domain of the URL.
     */
    const std::string& fulldomain() const noexcept;
    /**
     * @brief Get the port of the URL.
     * @return int port of the URL.
     */
    const int port() const noexcept;
    /**
     * @brief Get the parameters of the URL.
     * @return QueryParams parameters of the URL.
     */
    QueryParams params() const noexcept;
    /**
     * @brief Get the host of the URL.
     * @return TLD::Host object of the URL.
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
    static Host fromUrl(const std::string& url,
                        const bool ignore_www = DEFAULT_IGNORE_WWW);
    static void loadPslFromPath(const std::string& filepath);
    static void loadPslFromString(const std::string& filestr);
    /**
     * @brief Check if the Public Suffix List (PSL) is loaded.
     * @return true if the PSL is loaded, false otherwise.
     */
    static bool isPslLoaded() noexcept;
    static std::string_view removeWWW(const std::string_view&) noexcept;

   public:
    Host(const std::string& host, const bool ignore_www = DEFAULT_IGNORE_WWW);
    Host() = default;

    bool operator==(const Host&) const;
    bool operator==(const std::string&) const;

    const std::string& suffix() const noexcept;
    const std::string& domain() const noexcept;
    std::string domainName() const noexcept;
    const std::string& subdomain() const noexcept;
    const std::string& fulldomain() const noexcept;
    const std::string& str() const noexcept;

   private:
    class Impl;
    std::shared_ptr<Impl> impl; // since all methods are constants
};
}  // namespace TLD

std::ostream& operator<<(std::ostream& os, const TLD::QueryParams& dt);

std::ostream& operator<<(std::ostream& os, const TLD::Url& dt);

std::ostream& operator<<(std::ostream& os, const TLD::Host& dt);
#endif  // TLD_URLPARSER_H
