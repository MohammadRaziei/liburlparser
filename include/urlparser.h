/**

@file UrlParser.h
@brief This file contains the declaration of the UrlParser namespace and the Url
and Host classes. The UrlParser namespace provides functionality to parse and
extract information from URLs. The Url class represents a URL and provides
methods to access its various components such as protocol, subdomain, domain,
suffix, query, fragment, userinfo, abspath, port, params, and host. The Host
class represents the host part of a URL and provides methods to access its
subdomain, domain, suffix, and fulldomain.
*/
#ifndef DOMAINEXTRACTOR_URLPARSER_H
#define DOMAINEXTRACTOR_URLPARSER_H

#include <iostream>
#include <string>
#include <vector>

#ifndef PUBLIC_SUFFIX_LIST_DAT
#define PUBLIC_SUFFIX_LIST_DAT "public_suffix_list.dat"
#endif

namespace TLD {
/**
 * @typedef QueryParams
 * @brief A type alias for std::vectorstd::string.
 *
 * QueryParams represents the query parameters of a URL as a vector of strings.
 */
using QueryParams = std::vector<std::string>;

/**
 * @class Url
 * @brief Represents a URL and provides methods to access its components.
 *
 * The Url class represents a URL and provides methods to access its various
 * components such as protocol, subdomain, domain, suffix, query, fragment,
 * userinfo, abspath, port, params, and host.
 */
class Url {
   public:
    /**
     * @class Host
     * @brief Represents the host part of a URL.
     *
     * The Host class represents the host part of a URL and provides methods to
     * access its subdomain, domain, suffix, and fulldomain.
     */
    class Host;

    /**
     * @brief Constructs a Url object from the given URL string.
     * @param url The URL string to be parsed.
     */
    Url(const std::string& url);

    /**
     * @brief Destructor for the Url object.
     */
    ~Url();

    /**
     * @brief Loads the public suffix list from the specified file path.
     * @param filepath The path to the public suffix list file.
     */
    static void loadPslFromPath(const std::string& filepath);

    /**
     * @brief Loads the public suffix list from the specified string.
     * @param filestr The content of the public suffix list as a string.
     */
    static void loadPslFromString(const std::string& filestr);

    /**
     * @brief Checks if the public suffix list is loaded.
     * @return True if the public suffix list is loaded, false otherwise.
     */
    bool isPslLoaded() const noexcept;

    /**
     * @brief Returns the URL as a string.
     * @return The URL string.
     */
    std::string str() const;

    /**
     * @brief Returns the protocol part of the URL.
     * @return The protocol string.
     */
    std::string protocol() const;

    /**
     * @brief Returns the subdomain part of the URL.
     * @return The subdomain string.
     */
    std::string subdomain() const;

    /**
     * @brief Returns the domain part of the URL.
     * @return The domain string.
     */
    std::string domain() const;

    /**
     * @brief Returns the suffix part of the URL.
     * @return The suffix string.
     */
    std::string suffix() const;

    /**
     * @brief Returns the query part of the URL.
     * @return The query string.
     */
    std::string query() const;

    /**
     * @brief Returns the fragment part of the URL.
     * @return The fragment string.
     */
    std::string fragment() const;

    /**
     * @brief Returns the userinfo part of the URL.
     * @return The userinfo string.
     */
    std::string userinfo() const;

    /**
     * @brief Returns the absolute path part of the URL.
     * @return The absolute path string.
     */
    std::string abspath() const;

    /**
     * @brief Returns the domain name (domain + suffix) of the URL.
     * @return The domain name string.
     */
    std::string domainName() const;

    /**
     * @brief Returns the full domain (subdomain + domain + suffix) of the URL.
     * @return The full domain string.
     */
    std::string fulldomain() const;

    /**
     * @brief Returns the port number specified in the URL.
     * @return The port number.
     */
    int port() const;

    /**
     * @brief Returns the query parameters of the URL.
     * @return The query parameters as a QueryParams object.
     */
    QueryParams params() const;

    /**
     * @brief Returns the host part of the URL.
     * @return The Host object representing the host part.
     */
    Host host() const;

   protected:
    class Impl;
    Impl* impl;

    /**
     * @brief Constructs a Url object from the given implementation.
     * @param url_impl The implementation object.
     */
    Url(const Impl& url_impl);
};

/**
 * @class Host
 * @brief Represents the host part of a URL.
 *
 * The Host class represents the host part of a URL and provides methods to
 * access its subdomain, domain, suffix, and fulldomain.
 */
class Url::Host {
   public:
    /**
     * @brief Constructs a Host object from the given host string.
     * @param host The host string.
     */
    Host(const std::string& host);

    /**
     * @brief Returns the suffix part of the host.
     * @return The suffix string.
     */
    std::string suffix() const noexcept;

    /**
     * @brief Returns the domain part of the host.
     * @return The domain string.
     */
    std::string domain() const noexcept;

    /**
     * @brief Returns the domain name (domain + suffix) of the host.
     * @return The domain name string.
     */
    std::string domainName() const noexcept;

    /**
     * @brief Returns the subdomain part of the host.
     * @return The subdomain string.
     */
    std::string subdomain() const noexcept;

    /**
     * @brief Returns the full domain (subdomain + domain + suffix) of the host.
     * @return The full domain string.
     */
    std::string fulldomain() const noexcept;

    /**
     * @brief Returns the host as a string.
     * @return The host string.
     */
    std::string str() const noexcept;

   protected:
    std::string host_;
    std::string domain_;
    std::string subdomain_;
    std::string suffix_;
};
}  // namespace TLD

/**

@brief Overload of the output stream operator for QueryParams.
@param os The output stream.
@param dt The QueryParams object.
@return The output stream.
*/
std::ostream& operator<<(std::ostream& os, const TLD::QueryParams& dt);
/**

@brief Overload of the output stream operator for Url.
@param os The output stream.
@param dt The Url object.
@return The output stream.
*/
std::ostream& operator<<(std::ostream& os, const TLD::Url& dt);
/**

@brief Overload of the output stream operator for Host.
@param os The output stream.
@param dt The Host object.
@return The output stream.
*/
std::ostream& operator<<(std::ostream& os, const TLD::Url::Host& dt);
#endif  // DOMAINEXTRACTOR_URLPARSER_H
