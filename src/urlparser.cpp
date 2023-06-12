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
   public:
    static void loadPslFromPath(const std::string& filepath);
    static void loadPslFromString(const std::string& filestr);

   public:
    Impl(const std::string& url);
    ~Impl();
    static bool isPslLoaded() noexcept;
    std::string str() const;
    Host host() const;

   protected:
    static URL::PSL* psl;
    friend class Host;
    Host host_obj;
};


inline std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> strings;
    size_t start;
    size_t end = 0;
    while ((start = str.find_first_not_of(delim, end)) != std::string::npos) {
        end = str.find(delim, start);
        strings.push_back(str.substr(start, end - start));
    }
    return strings;
}

URL::PSL* TLD::Url::Impl::psl = new URL::PSL(URL::PSL::fromPath(PUBLIC_SUFFIX_LIST_DAT));

inline void TLD::Url::Impl::loadPslFromPath(const std::string& filepath) {
    delete psl;
    psl = new URL::PSL(URL::PSL::fromPath(filepath));
}

inline void TLD::Url::Impl::loadPslFromString(const std::string& filestr) {
    delete psl;
    psl =  new URL::PSL(URL::PSL::fromString(filestr));
}

inline bool TLD::Url::Impl::isPslLoaded() noexcept {
    return psl != nullptr;
}

TLD::Url::Host::Host(const std::string& host_) : host_(host_) {
    this->suffix_ = TLD::Url::Impl::psl->getTLD(host_);
    size_t suffix_pos = host_.rfind("." + suffix_);
    if (suffix_pos == std::string::npos || suffix_pos < 1)
        return;
    subdomain_ = host_.substr(0, suffix_pos);
    size_t domain_pos = subdomain_.find_last_of(".");
    this->domain_ = subdomain_.substr(domain_pos + 1);
    this->subdomain_ = subdomain_.substr(0, domain_pos);
}
inline std::string TLD::Url::Host::suffix() const noexcept {
    return suffix_;
}
inline std::string TLD::Url::Host::domain() const noexcept {
    return domain_;
}
inline std::string TLD::Url::Host::subdomain() const noexcept {
    return subdomain_;
}

inline std::string TLD::Url::Host::str() const noexcept {
    return host_;
}

inline std::string TLD::Url::Host::fulldomain() const noexcept {
    return host_;
}

std::string TLD::Url::Host::domainName() const noexcept {
    return domain_ + "." + suffix_;
}

TLD::Url::Host TLD::Url::Host::from_url(const std::string& url) {
    return TLD::Url::Host(TLD::Url(url).host()); // TODO :  write a faster way for finding host_ from url
}

TLD::Url::Impl::Impl(const std::string& url)
    : URL::Url(url), host_obj(this->host_) {}

TLD::Url::Impl::~Impl() {}

TLD::Url::Url(const std::string& url) : impl(new Impl(url)) {}

TLD::Url::~Url() {
    delete impl;
}

void TLD::Url::loadPslFromPath(const std::string& filepath) {
    TLD::Url::Impl::loadPslFromPath(filepath);
}

void TLD::Url::loadPslFromString(const std::string& filestr) {
    TLD::Url::Impl::loadPslFromString(filestr);
}

inline std::string TLD::Url::Impl::str() const {
    return URL::Url::str();
}

inline TLD::Url::Host TLD::Url::Impl::host() const {
    return Url::host();
}

std::string TLD::Url::str() const {
    return impl->str();
}

std::string TLD::Url::domain() const {
    return impl->host().domain();
}

std::string TLD::Url::subdomain() const {
    return impl->host().subdomain();
}

std::string TLD::Url::suffix() const {
    return impl->host().suffix();
}

std::string TLD::Url::protocol() const {
    return impl->scheme();
}

int TLD::Url::port() const {
    return impl->port();
}

std::string TLD::Url::fulldomain() const {
    return impl->host().fulldomain();
}

std::string TLD::Url::domainName() const {
    return impl->host().domainName();
}

TLD::Url::Host TLD::Url::host() const {
    return impl->host();
}

std::string TLD::Url::query() const {
    return impl->query();
}

std::string TLD::Url::fragment() const {
    return impl->fragment();
}

std::string TLD::Url::userinfo() const {
    return impl->userinfo();
}

std::string TLD::Url::abspath() const {
    return impl->abspath().str();
}

TLD::QueryParams TLD::Url::params() const {
    return split(query(), '&');
}

TLD::Url::Url(const TLD::Url::Impl& url_impl)
    : impl(new TLD::Url::Impl(url_impl)) {}

bool TLD::Url::isPslLoaded() const noexcept {
    return impl->isPslLoaded();
}

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
std::ostream& operator<<(std::ostream& os, const TLD::Url::Host& dt) {
    os << dt.str();
    return os;
}
