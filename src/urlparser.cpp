//
// Created by mohammad on 5/20/23.
//
#include <iostream>

#include "urlparser.h"
#include "psl.h"
#include "url.h"

namespace URL = Url;

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


class TLD::Url::Impl : public URL::Url{
//    friend class TLD::Url;
public:
    static void loadPslFromPath(const std::string& filepath);
    static void loadPslFromString(const std::string& filestr);
public:
//    Impl();
    Impl(const std::string& url);
    ~Impl();

    std::string suffix() { return suffix_; }
    std::string domain() { return domain_; }
    std::string subdomain() { return subdomain_; }
    bool isPslLoaded() const noexcept;

protected:
    static URL::PSL psl;

    std::string suffix_;
    std::string domain_;
    std::string subdomain_;
};


URL::PSL TLD::Url::Impl::psl = URL::PSL::fromPath(PUBLIC_SUFFIX_LIST_DAT);


//TLD::Url::Impl::loadPslFromPath("public_suffix_list.dat");

std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> strings;
    size_t start;
    size_t end = 0;
    while ((start = str.find_first_not_of(delim, end)) != std::string::npos) {
        end = str.find(delim, start);
        strings.push_back(str.substr(start, end - start));
    }
    return strings;
}

void TLD::Url::Impl::loadPslFromPath(const std::string& filepath) {
    psl = URL::PSL::fromPath(filepath);
}

void TLD::Url::Impl::loadPslFromString(const std::string& filestr) {
    psl = URL::PSL::fromString(filestr);
}


//TLD::Url::Impl::Impl() {
//
//}
bool TLD::Url::Impl::isPslLoaded() const noexcept {
    std::cout << psl.numLevels() << std::endl;
    return psl.numLevels() > 0;
}

TLD::Url::Impl::Impl(const std::string& url): URL::Url(url) {
    this->suffix_ = psl.getTLD(host_);
    size_t suffix_pos = host_.rfind("." + suffix_);
    if (suffix_pos == std::string::npos || suffix_pos < 1) return;
    subdomain_ = host_.substr(0, suffix_pos);
    size_t domain_pos = subdomain_.find_last_of(".");
    this->domain_ = subdomain_.substr(domain_pos + 1);
    this->subdomain_ = subdomain_.substr(0, domain_pos);
}

TLD::Url::Impl::~Impl() {

}



TLD::Url::Url(const std::string& url) : impl(new Impl(url)) {
}

TLD::Url::~Url(){
    delete impl;
}

void TLD::Url::loadPslFromPath(const std::string &filepath){
    TLD::Url::Impl::loadPslFromPath(filepath);
}

void TLD::Url::loadPslFromString(const std::string &filestr){
    TLD::Url::Impl::loadPslFromString(filestr);
}


std::string TLD::Url::str() const{
    return impl->str();
}

std::string TLD::Url::domain() const{
    return impl->domain(); // TODO
}

std::string TLD::Url::subdomain() const{
    return impl->subdomain(); // TODO
}

std::string TLD::Url::suffix() const{
    return impl->suffix();
}


std::string TLD::Url::protocol() const{
    return impl->scheme();
}


int TLD::Url::port() const{
    return impl->port();
}


std::string TLD::Url::host() const{
    return impl->host();
}

std::string TLD::Url::query() const{
    return impl->query();
}

std::string TLD::Url::fragment() const{
    return impl->fragment();
}

std::string TLD::Url::userinfo() const{
    return impl->userinfo();
}

std::string TLD::Url::abspath() const{
    return impl->abspath().str();
}

TLD::QueryParams TLD::Url::params() const{
    return split(query(), '&');
}

TLD::Url::Url(const TLD::Url::Impl &url_impl) : impl(new TLD::Url::Impl(url_impl)){
}

bool TLD::Url::isPslLoaded() const noexcept {
    return impl->isPslLoaded();
}
