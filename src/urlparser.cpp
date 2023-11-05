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
    Impl(const std::string &url, const bool ignore_www);

    ~Impl() {}

    const TLD::Host &host() const noexcept;

protected:
    TLD::Host host_obj;
};

class TLD::Host::Impl {
    friend class TLD::Host;

public:
    static void loadPslFromPath(const std::string &filepath);

    static void loadPslFromString(const std::string &filestr);

    static bool isPslLoaded() noexcept;

public:
    Impl(const std::string &host, const bool ignore_www);

    ~Impl() {}

    const std::string &domain() const noexcept;

    std::string domainName() const noexcept;

    const std::string &subdomain() const noexcept;

    const std::string &suffix() const noexcept;

    const std::string &fulldomain() const noexcept;

    const std::string &str() const noexcept;

protected:
    std::string host_;
    std::string domain_;
    std::string subdomain_;
    std::string suffix_;
    std::string fulldomain_;
    static URL::PSL *psl;
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

URL::PSL *initiate_static_psl() {
    URL::PSL *psl;
    try {
        psl = new URL::PSL(URL::PSL::fromPath(PUBLIC_SUFFIX_LIST_DAT));
    }
    catch (const std::invalid_argument &) {
        psl = new URL::PSL(URL::PSL::fromString(""));
    }
    return psl;
}

#ifndef DONT_INIT_PSL
URL::PSL *TLD::Host::Impl::psl = initiate_static_psl();
#else
URL::PSL* TLD::Host::Impl::psl = new URL::PSL(URL::PSL::fromString(""));
#endif

inline void TLD::Host::Impl::loadPslFromPath(const std::string &filepath) {
    delete psl;
    psl = new URL::PSL(URL::PSL::fromPath(filepath));
}

inline void TLD::Host::Impl::loadPslFromString(const std::string &filestr) {
    delete psl;
//    if (!filestr.empty())
    psl = new URL::PSL(URL::PSL::fromString(filestr));
}

void TLD::Host::loadPslFromPath(const std::string &filepath) {
    TLD::Host::Impl::loadPslFromPath(filepath);
}

void TLD::Host::loadPslFromString(const std::string &filestr) {
    TLD::Host::Impl::loadPslFromString(filestr);
}

inline bool TLD::Host::Impl::isPslLoaded() noexcept {
    return psl != nullptr && psl->numLevels() > 0;
}

bool TLD::Host::isPslLoaded() noexcept {
    return TLD::Host::Impl::isPslLoaded();
}

bool TLD::Url::isPslLoaded() noexcept {
    return TLD::Host::isPslLoaded();
}
////////////////////////////////////////////////////////////////////

TLD::Host::Impl::Impl(const std::string &host_, const bool ignore_www) : host_(host_) {
    this->suffix_ = TLD::Host::Impl::psl->getTLD(host_);
    this->fulldomain_ = host_;
    size_t suffix_pos = fulldomain_.rfind("." + suffix_);
    size_t subdomain_pos = 0;
    if (suffix_pos == std::string::npos || suffix_pos < 1)
        return;
    domain_ = host_.substr(0, suffix_pos);
    size_t domain_pos = domain_.find_last_of('.');
    if (domain_pos != std::string::npos) {
        if (ignore_www) {
            size_t www_pos = domain_.find("www.");
            if (www_pos != 0) {
                if (www_pos != std::string::npos)
                    return;
            } else {
                subdomain_pos = 4;
                fulldomain_ = fulldomain_.substr(4);
            }
        }
        if (subdomain_pos < domain_pos)
            subdomain_ = domain_.substr(subdomain_pos, domain_pos - subdomain_pos);
        domain_ = domain_.substr(domain_pos + 1);
    }

}

TLD::Host::Host(const std::string &url, const bool ignore_www) : impl(new Impl(url, ignore_www)) {}

TLD::Host::Host(TLD::Host &&host) noexcept: impl(host.impl) {
    host.impl = nullptr;
}

TLD::Host::~Host() {
    delete impl;
}


TLD::Url::Impl::Impl(const std::string &url, const bool ignore_www)
        : URL::Url(url), host_obj(TLD::Host{this->host_, ignore_www}) {
}

TLD::Url::Url(const std::string &url, const bool ignore_www) :
        impl(new Impl(url, ignore_www)) {
}

TLD::Url::Url(TLD::Url &&url) noexcept: impl(url.impl) {
    url.impl = nullptr;
}

TLD::Url::~Url() {
    delete impl;
}

//////////////////////////////////////////////////////////////////////
const TLD::Host &TLD::Url::Impl::host() const noexcept {
    return this->host_obj;  // TODO
}

const TLD::Host &TLD::Url::host() const {
    //    return impl->host_obj;
    return impl->host_obj;  // TODO
}

/// suffix:
inline const std::string &TLD::Host::Impl::suffix() const noexcept {
    return suffix_;
}

const std::string &TLD::Host::suffix() const noexcept {
    return impl->suffix();
}

const std::string &TLD::Url::suffix() const noexcept {
    return impl->host_obj.suffix();
}

/// subdomain
inline const std::string &TLD::Host::Impl::subdomain() const noexcept {
    return subdomain_;
}

const std::string &TLD::Host::subdomain() const noexcept {
    return impl->subdomain();
}

const std::string &TLD::Url::subdomain() const noexcept {
    return impl->host_obj.subdomain();
}

/// domain
inline const std::string &TLD::Host::Impl::domain() const noexcept {
    return domain_;
}

const std::string &TLD::Host::domain() const noexcept {
    return impl->domain();
}

const std::string &TLD::Url::domain() const noexcept {
    return impl->host_obj.domain();
}

/// fulldomain
const std::string &TLD::Host::Impl::fulldomain() const noexcept {
    return fulldomain_;
}

const std::string &TLD::Host::fulldomain() const noexcept {
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
inline const std::string &TLD::Host::Impl::str() const noexcept {
    return fulldomain();
}

const std::string &TLD::Host::str() const noexcept {
    return impl->str();
}


std::string TLD::Url::str() const noexcept {
    return impl->str();
}

TLD::Host TLD::Host::fromUrl(const std::string &url, const bool ignore_www) {
    return TLD::Host(TLD::Url::extractHost(url), ignore_www);
}

TLD::Host::Host(const TLD::Host &host) : impl(new Impl(*host.impl)) {}

////////////////////////

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

TLD::Url::Url(const TLD::Url &url) : impl(new Impl(*url.impl)) {}

TLD::Url &TLD::Url::operator=(const TLD::Url &url) {
    delete impl;
    impl = new TLD::Url::Impl(*url.impl);
    return *this;
}

TLD::Url &TLD::Url::operator=(TLD::Url &&url) {
    delete impl;
    impl = url.impl;
    url.impl = nullptr;
    return *this;
}

//bool TLD::Url::autoCorrect(std::string &url) noexcept {
//    size_t qmark = url.find_first_of('?');
//    if (qmark < 4 or qmark==std::string::npos) return false;
//    size_t slash = url.find_last_of('/', qmark - 1);
//    if (slash > 4 and slash != std::string::npos and url[slash - 1] == '/') {
//        url.insert(qmark, 1, '/');
//        return true;
//    }
//    return false;
//}

std::string TLD::Url::extractHost(const std::string &url) noexcept {
    // TODO: cannot not handling https:// in general (with or without)
    std::string host;
    size_t pos = url.find("://");
    if (pos != std::string::npos) {
        pos += 3; // skip over "://"
    } else {
        pos = 0;
    }
    size_t at_pos = url.find_first_of('@', pos);
    size_t end_pos = url.find_first_of("?/", pos);
    if (end_pos == std::string::npos) {
        end_pos = url.length();
    }
    if (at_pos < end_pos) {
        pos = at_pos + 1;
    }
    host = url.substr(pos, end_pos - pos);
    return host;
}

TLD::Host &TLD::Host::operator=(const TLD::Host &host) {
    if (this == &host) { return *this; }
    *impl = *host.impl;
    return *this;
}

TLD::Host &TLD::Host::operator=(TLD::Host &&host) noexcept {
    if (impl == host.impl) { return *this; }
    delete impl;
    impl = host.impl;
    host.impl = nullptr;
    return *this;
}

bool TLD::Host::operator==(const TLD::Host &host) const {
    return impl->fulldomain() == host.impl->fulldomain();
}

bool TLD::Host::operator==(const std::string &host) const {
    return impl->fulldomain() == host;
}

std::ostream &operator<<(std::ostream &os, const TLD::QueryParams &v) {
    os << "[";
    for (const auto &e: v) {
        os << e << ", ";
    }
    os << (v.empty() ? "" : "\b\b") << "]";
    return os;
}

std::ostream &operator<<(std::ostream &os, const TLD::Url &dt) {
    os << dt.str();
    return os;
}

std::ostream &operator<<(std::ostream &os, const TLD::Host &dt) {
    os << dt.str();
    return os;
}
