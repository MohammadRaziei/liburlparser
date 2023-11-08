//
// Created by mohammad on 5/20/23.
//
#include "urlparser.h"

#include <iostream>

#include "url.h"

namespace URL = Url;

/// define Impl class:
class TLD::Url::Impl : public URL::Url {
    friend class TLD::Url;

public:
    Impl(const std::string &url, const bool ignore_www);

    const std::shared_ptr<TLD::Host> getHost() noexcept;

private:
    std::shared_ptr<TLD::Host> host_obj = nullptr;
    const bool ignore_www;
};

inline std::vector<std::string> split(const std::string &str,
                                      char delim) noexcept {
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

bool TLD::Url::isPslLoaded() noexcept {
    return TLD::Host::isPslLoaded();
}
////////////////////////////////////////////////////////////////////


TLD::Url::Impl::Impl(const std::string &url, const bool ignore_www)
        : URL::Url(url), ignore_www(ignore_www) {
}

TLD::Url::Url(const std::string &url, const bool ignore_www) :
        impl(std::make_unique<TLD::Url::Impl>(url, ignore_www)) {
}

TLD::Url::Url(const TLD::Url &url) :
        impl(std::make_unique<TLD::Url::Impl>(*url.impl)) {
}

//TLD::Url::Url(TLD::Url &&url) noexcept : impl(std::move(url.impl)) {}

TLD::Url::~Url() noexcept {}


TLD::Url &TLD::Url::operator=(const TLD::Url &other) {
    if (this == &other) return *this;
    impl = std::make_unique<Impl>(*other.impl);
    return *this;
}

//TLD::Url &TLD::Url::operator=(TLD::Url &&other) noexcept {
//    impl = std::move(other.impl);
//    return *this;
//}



const std::shared_ptr<TLD::Host> TLD::Url::Impl::getHost() noexcept {
    if (!host_obj)
        host_obj = std::make_shared<TLD::Host>(host_, ignore_www);
    return host_obj;
}

const TLD::Host &TLD::Url::host() const {
    return *impl->getHost();
}

/// suffix
const std::string &TLD::Url::suffix() const noexcept {
    return impl->getHost()->suffix();
}

/// subdomain
const std::string &TLD::Url::subdomain() const noexcept {
    return impl->getHost()->subdomain();
}

/// domain
const std::string &TLD::Url::domain() const noexcept {
    return impl->getHost()->domain();
}

/// fulldomain
std::string TLD::Url::fulldomain() const noexcept {
    return impl->getHost()->fulldomain();
}

/// domainName
std::string TLD::Url::domainName() const noexcept {
    return impl->getHost()->domainName();
}

// str
std::string TLD::Url::str() const noexcept {
    return impl->str();
}

const std::string &TLD::Url::protocol() const noexcept {
    return impl->scheme();
}

const int TLD::Url::port() const noexcept {
    return impl->port();
}

std::string TLD::Url::query() const noexcept {
    return impl->query();
}

std::string TLD::Url::fragment() const noexcept {
    return impl->fragment();
}

std::string TLD::Url::userinfo() const noexcept {
    return impl->userinfo();
}

std::string TLD::Url::abspath() const noexcept {
    return impl->abspath().str();
}

TLD::QueryParams TLD::Url::params() const noexcept {
    return split(query(), '&');
}





std::string TLD::Url::extractHost(const std::string &url) noexcept {
    std::string host;
    size_t pos = url.find("://");
    if (pos != std::string::npos) {
        pos += 3; // skip over "://"
    } else {
        pos = 0;
    }
    size_t end_pos = url.find_first_of("?/", pos);
    if (end_pos == std::string::npos) {
        end_pos = url.length();
    }
    size_t at_pos = url.find_first_of('@', pos);
    if (at_pos < end_pos) {
        pos = at_pos + 1;
    }
    host = url.substr(pos, end_pos - pos);
    return host;
}

bool TLD::Url::operator==(const TLD::Url &other) const{
    return *impl == *other.impl;
}


std::ostream &operator<<(std::ostream &os, const TLD::QueryParams &v) {
    os << "[";
    for (const auto &e: v) {
        os << e << ", ";
    }
    os << (v.empty() ? "" : "\b\b") << "]";
    return os;
}

std::ostream &operator<<(std::ostream &os, const TLD::Host &dt) {
    os << dt.str();
    return os;
}

std::ostream &operator<<(std::ostream &os, const TLD::Url &dt) {
    os << dt.str();
    return os;
}

