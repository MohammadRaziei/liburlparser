#ifndef DOMAINEXTRACTOR_URLPARSER_H
#define DOMAINEXTRACTOR_URLPARSER_H

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

class Url {
   public:
    static bool isPslLoaded() noexcept;
    static std::string extractHost(const std::string& url) noexcept;

   public:
    Url(const std::string& url, const bool ignore_www = DEFAULT_IGNORE_WWW);
    Url() noexcept = default;

    bool operator==(const Url&) const;

    std::string str() const noexcept;
    const std::string& protocol() const noexcept;
    const std::string& subdomain() const noexcept;
    const std::string& domain() const noexcept;
    const std::string& suffix() const noexcept;
    const std::string& query() const noexcept;
    const std::string& fragment() const noexcept;
    const std::string& userinfo() const noexcept;
    std::string abspath() const noexcept;
    std::string domainName() const noexcept;
    const std::string& fulldomain() const noexcept;
    const int port() const noexcept;
    QueryParams params() const noexcept;
    const Host& host() const;

   private:
    class Impl;
    std::shared_ptr<Impl> impl; // since all methods are constants
};

class Host {
   public:
    static Host fromUrl(const std::string& url,
                        const bool ignore_www = DEFAULT_IGNORE_WWW);
    static void loadPslFromPath(const std::string& filepath);
    static void loadPslFromString(const std::string& filestr);
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
#endif  // DOMAINEXTRACTOR_URLPARSER_H
