#ifndef DOMAINEXTRACTOR_URLPARSER_H
#define DOMAINEXTRACTOR_URLPARSER_H

#include <iostream>
#include <string>
#include <vector>
#include <memory>

#ifndef PUBLIC_SUFFIX_LIST_DAT
#define PUBLIC_SUFFIX_LIST_DAT "public_suffix_list.dat"
#endif

namespace TLD {

using QueryParams = std::vector<std::string>;

class Host;
class Url {
   public:
    static void loadPslFromPath(const std::string& filepath);
    static void loadPslFromString(const std::string& filestr);
    static bool isPslLoaded() noexcept;
   public:
    Url(const std::string& url);
    Url(const Url& url);
    Url(Url&& url) = default;
    ~Url();

    std::string str() const noexcept;
    std::string protocol() const noexcept;
    std::string subdomain() const noexcept;
    std::string domain() const noexcept;
    std::string suffix() const noexcept;
    std::string query() const noexcept;
    std::string fragment() const noexcept;
    std::string userinfo() const noexcept;
    std::string abspath() const noexcept;
    std::string domainName() const noexcept;
    std::string fulldomain() const noexcept;
    int port() const noexcept;
    QueryParams params() const noexcept;
    const Host& host() const;

   protected:
    class Impl;
    std::unique_ptr<Impl> impl;
    Url(const Impl& url_impl);
};

class Host {
   public:
    static const Host& from_url(const std::string& url);
    static void loadPslFromPath(const std::string& filepath);
    static void loadPslFromString(const std::string& filestr);
    static bool isPslLoaded() noexcept;

   public:
    Host(const std::string& host);
    Host(const Host& host);
    Host(Host&& host) = default;
    ~Host();
    std::string suffix() const noexcept;
    std::string domain() const noexcept;
    std::string domainName() const noexcept;
    std::string subdomain() const noexcept;
    std::string fulldomain() const noexcept;
    std::string str() const noexcept;

   protected:
    class Impl;
    std::unique_ptr<Impl> impl;
};
}  // namespace TLD

std::ostream& operator<<(std::ostream& os, const TLD::QueryParams& dt);

std::ostream& operator<<(std::ostream& os, const TLD::Url& dt);

std::ostream& operator<<(std::ostream& os, const TLD::Host& dt);
#endif  // DOMAINEXTRACTOR_URLPARSER_H
