//
// Created by mohammad on 5/20/23.
//
#include "urlparser.h"

#include <iostream>

#include "psl.h"
#include "url.h"

namespace URL = Url;

/// define Impl class:
class TLD::Url::Impl : public URL::Url {
    friend class TLD::Url;
   public:
    static void loadPslFromPath(const std::string& filepath);
    static void loadPslFromString(const std::string& filestr);
    static bool isPslLoaded() noexcept;

   public:
    Impl(const std::string& url);
    ~Impl(){}
    std::string str() const noexcept;
    const TLD::Host& host() const noexcept;

   protected:
    TLD::Host host_obj;
};


class TLD::Host::Impl{
    friend class TLD::Host;
   public:
    static void loadPslFromPath(const std::string& filepath);
    static void loadPslFromString(const std::string& filestr);
    static bool isPslLoaded() noexcept;

   public:
    Impl(const std::string& host);
    ~Impl(){}

    std::string host() const noexcept;
    std::string domain() const noexcept;
    std::string domainName() const noexcept;
    std::string subdomain() const noexcept;
    std::string suffix() const noexcept;
    std::string fulldomain() const noexcept;
    std::string str() const noexcept;

   protected:
    std::string host_;
    std::string domain_;
    std::string subdomain_;
    std::string suffix_;
    static URL::PSL* psl;
};

inline std::vector<std::string> split(const std::string& str, char delim) noexcept{
    std::vector<std::string> strings;
    size_t start;
    size_t end = 0;
    while ((start = str.find_first_not_of(delim, end)) != std::string::npos) {
        end = str.find(delim, start);
        strings.push_back(str.substr(start, end - start));
    }
    return strings;
}

////////////////////////////////////////////////////////////////////////////////////////
URL::PSL* TLD::Host::Impl::psl = new URL::PSL(URL::PSL::fromPath(PUBLIC_SUFFIX_LIST_DAT));


inline void TLD::Host::Impl::loadPslFromPath(const std::string& filepath) {
    delete psl;
    psl = new URL::PSL(URL::PSL::fromPath(filepath));
}

inline void TLD::Host::Impl::loadPslFromString(const std::string& filestr) {
    delete psl;
    if (!filestr.empty())
        psl =  new URL::PSL(URL::PSL::fromString(filestr));
}

void TLD::Host::loadPslFromPath(const std::string& filepath) {
    TLD::Host::Impl::loadPslFromPath(filepath);
}

void TLD::Host::loadPslFromString(const std::string& filestr) {
    TLD::Host::Impl::loadPslFromString(filestr);
}

void TLD::Url::loadPslFromPath(const std::string& filepath) {
    TLD::Host::loadPslFromPath(filepath);
}

void TLD::Url::loadPslFromString(const std::string& filestr) {
    TLD::Host::loadPslFromString(filestr);
}


inline bool TLD::Host::Impl::isPslLoaded() noexcept {
    return psl != nullptr;
}
bool TLD::Host::isPslLoaded() noexcept {
    return TLD::Host::Impl::isPslLoaded();
}
bool TLD::Url::isPslLoaded() noexcept {
    return TLD::Host::isPslLoaded();
}

////////////////////////////////////////////////////////////////////



TLD::Host::Impl::Impl(const std::string& host_) : host_(host_) {
    this->suffix_ = TLD::Host::Impl::psl->getTLD(host_);
    size_t suffix_pos = host_.rfind("." + suffix_);
    if (suffix_pos == std::string::npos || suffix_pos < 1)
        return;
    subdomain_ = host_.substr(0, suffix_pos);
    size_t domain_pos = subdomain_.find_last_of(".");
    this->domain_ = subdomain_.substr(domain_pos + 1);
    this->subdomain_ = subdomain_.substr(0, domain_pos);
}
TLD::Host::Host(const std::string& url) : impl(new Impl(url)) {}
//TLD::Host::Host(TLD::Host&& host) : impl(host.impl){ delete host.impl; }
TLD::Host::~Host() {
//    delete impl;
}

TLD::Url::Impl::Impl(const std::string& url) : URL::Url(url), host_obj(this->host_) {}
//TLD::Url::Impl::~Impl() {}
TLD::Url::Url(const std::string& url) : impl(new Impl(url)) {}
TLD::Url::~Url() {
//    delete impl;
}

//////////////////////////////////////////////////////////////////////
const TLD::Host& TLD::Url::Impl::host() const noexcept{
    return this->host_obj; // TODO
}
const TLD::Host& TLD::Url::host() const{
//    return impl->host_obj;
    return impl->host_obj; // TODO
}
std::string TLD::Host::Impl::host() const noexcept{
    return host_;
}


/// suffix:
inline std::string TLD::Host::Impl::suffix() const noexcept {
    return suffix_;
}
std::string TLD::Host::suffix() const noexcept {
    return impl->suffix();
}
std::string TLD::Url::suffix() const noexcept {
    return impl->host_obj.suffix();
}
/// subdomain
inline std::string TLD::Host::Impl::subdomain() const noexcept {
    return subdomain_;
}
std::string TLD::Host::subdomain() const noexcept {
    return impl->subdomain();
}
std::string TLD::Url::subdomain() const noexcept {
    return impl->host_obj.subdomain();
}
/// domain
inline std::string TLD::Host::Impl::domain() const noexcept {
    return domain_;
}
std::string TLD::Host::domain() const noexcept {
    return impl->domain();
}
std::string TLD::Url::domain() const noexcept {
    return impl->host_obj.domain();
}
/// fulldomain
inline std::string TLD::Host::Impl::fulldomain() const noexcept {
    return host_;
}
std::string TLD::Host::fulldomain() const noexcept {
    return impl->fulldomain();
}
std::string TLD::Url::fulldomain() const noexcept {
    return impl->host_obj.fulldomain();
}
/// domainName
inline std::string TLD::Host::Impl::domainName() const noexcept {
    return domain_ + "." + suffix_;
}
std::string TLD::Host::domainName() const noexcept {
    return impl->domainName();
}
std::string TLD::Url::domainName() const noexcept {
    return impl->host_obj.domainName();
}
// str
inline std::string TLD::Host::Impl::str() const noexcept {
    return host_;
}
std::string TLD::Host::str() const noexcept {
    return impl->str();
}
inline std::string TLD::Url::Impl::str() const noexcept{
    return URL::Url::str();
}
std::string TLD::Url::str() const noexcept {
    return impl->str();
}



const TLD::Host& TLD::Host::from_url(const std::string& url) {
    return std::move(TLD::Url(url).host()); // TODO :  write a faster way for finding host_ from url
}

TLD::Host::Host(const TLD::Host &host) : impl(new Impl(*host.impl)){
}


////////////////////////


std::string TLD::Url::protocol() const noexcept{
    return impl->scheme();
}

int TLD::Url::port() const noexcept{
    return impl->port();
}

std::string TLD::Url::query() const noexcept{
    return impl->query();
}

std::string TLD::Url::fragment() const noexcept{
    return impl->fragment();
}

std::string TLD::Url::userinfo() const noexcept{
    return impl->userinfo();
}

std::string TLD::Url::abspath() const noexcept{
    return impl->abspath().str();
}

TLD::QueryParams TLD::Url::params() const noexcept{
    return split(query(), '&');
}

TLD::Url::Url(const TLD::Url &url) : impl(new Impl(*url.impl)){

}


/*
 */

std::ostream& operator<<(std::ostream& os, const TLD::QueryParams& v) {
    os << "[";
    for (auto& e : v) {
        os << e << ", ";
    }
    os << (v.empty() ? "" : "\b\b") << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const TLD::Url& dt) {
    os << dt.str();
    return os;
}
std::ostream& operator<<(std::ostream& os, const TLD::Host& dt) {
    os << dt.str();
    return os;
}
