//
// Created by mohammad on 5/20/23.
//

#ifndef DOMAINEXTRACTOR_URLPARSER_H
#define DOMAINEXTRACTOR_URLPARSER_H

#include <string>
#include <vector>
#include <iostream>

#ifndef PUBLIC_SUFFIX_LIST_DAT
#define PUBLIC_SUFFIX_LIST_DAT "public_suffix_list.dat"
#endif


namespace TLD {
//    class QueryParams : public std::vector<std::string>{};
    using QueryParams = std::vector<std::string>;

    class Url {

    public:
        Url(const std::string& url);
        ~Url();

        static void loadPslFromPath(const std::string& filepath);
        static void loadPslFromString(const std::string& filestr);
        bool isPslLoaded() const noexcept;


        std::string str() const;
        std::string protocol() const;
        std::string subdomain() const;
        std::string domain() const;
        std::string suffix() const;
        std::string host() const;
        std::string query() const;
        std::string fragment() const;
        std::string userinfo() const;
        std::string abspath() const;

        int port() const;
        QueryParams params() const;
//    void doSomething();
    protected:
        class Impl;
        Impl* impl;
        Url(const Impl& url_impl);
    };
}

std::ostream& operator<<(std::ostream& os, const TLD::QueryParams& dt);
std::ostream& operator<<(std::ostream& os, const TLD::Url& dt);



#endif //DOMAINEXTRACTOR_URLPARSER_H
