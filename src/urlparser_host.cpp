//
// Created by mohammad on 5/20/23.
//
#include "urlparser.h"

#include <iostream>

#include "psl.h"

namespace URL = Url;

class TLD::Host::Impl {
    friend class TLD::Host;

   public:
    static void loadPslFromPath(const std::string& filepath);
    static void loadPslFromString(const std::string& filestr);
    static bool isPslLoaded() noexcept;

   public:
    Impl(const std::string& host, const bool ignore_www);
    ~Impl() = default;

    const std::string& domain() const noexcept;
    std::string domainName() const noexcept;
    const std::string& subdomain() const noexcept;
    const std::string& suffix() const noexcept;
    const std::string& fulldomain() const noexcept;
    const std::string& str() const noexcept;

   private:
    std::string host_;
    std::string domain_;
    std::string subdomain_;
    std::string suffix_;
    std::string fulldomain_;
    static URL::PSL psl;
};

////////////////////////////////////////////////////////////////////////////////////////
#ifndef DONT_INIT_PSL
URL::PSL initiate_static_psl() {
    URL::PSL psl;
    try {
        psl = URL::PSL::fromPath(PUBLIC_SUFFIX_LIST_DAT);
    } catch (const std::invalid_argument&) {
        psl = URL::PSL::fromString("");
    }
    return psl;
}
URL::PSL TLD::Host::Impl::psl = initiate_static_psl();
#else
URL::PSL TLD::Host::Impl::psl = URL::PSL::fromString("");
#endif

inline void TLD::Host::Impl::loadPslFromPath(const std::string& filepath) {
    psl = URL::PSL::fromPath(filepath);
}

inline void TLD::Host::Impl::loadPslFromString(const std::string& filestr) {
    psl = URL::PSL::fromString(filestr);
}

void TLD::Host::loadPslFromPath(const std::string& filepath) {
    TLD::Host::Impl::loadPslFromPath(filepath);
}

void TLD::Host::loadPslFromString(const std::string& filestr) {
    TLD::Host::Impl::loadPslFromString(filestr);
}

inline bool TLD::Host::Impl::isPslLoaded() noexcept {
    return psl.numLevels() > 0;
}

bool TLD::Host::isPslLoaded() noexcept {
    return TLD::Host::Impl::isPslLoaded();
}
////////////////////////////////////////////////////////////////////

TLD::Host::Impl::Impl(const std::string& host_, const bool ignore_www)
    : host_(host_), fulldomain_(host_) {
    this->suffix_ = TLD::Host::Impl::psl.getTLD(host_);
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
                subdomain_pos = 4;  // length of "www."
                fulldomain_ = fulldomain_.substr(4);
            }
        }
        if (subdomain_pos < domain_pos)
            subdomain_ =
                domain_.substr(subdomain_pos, domain_pos - subdomain_pos);
        domain_ = domain_.substr(domain_pos + 1);
    }
}

TLD::Host::Host(const std::string& host, const bool ignore_www)
    : impl(std::make_shared<Impl>(host, ignore_www)) {}

/// suffix:
inline const std::string& TLD::Host::Impl::suffix() const noexcept {
    return suffix_;
}

const std::string& TLD::Host::suffix() const noexcept {
    return impl->suffix();
}

/// subdomain
inline const std::string& TLD::Host::Impl::subdomain() const noexcept {
    return subdomain_;
}

const std::string& TLD::Host::subdomain() const noexcept {
    return impl->subdomain();
}

/// domain
inline const std::string& TLD::Host::Impl::domain() const noexcept {
    return domain_;
}

const std::string& TLD::Host::domain() const noexcept {
    return impl->domain();
}

/// fulldomain
const std::string& TLD::Host::Impl::fulldomain() const noexcept {
    return fulldomain_;
}

const std::string& TLD::Host::fulldomain() const noexcept {
    return impl->fulldomain();
}

/// domainName
inline std::string TLD::Host::Impl::domainName() const noexcept {
    return domain_ + "." + suffix_;
}

std::string TLD::Host::domainName() const noexcept {
    return impl->domainName();
}

const std::string& TLD::Host::str() const noexcept {
    return impl->fulldomain();
}

TLD::Host TLD::Host::fromUrl(const std::string& url, const bool ignore_www) {
    return TLD::Host(TLD::Url::extractHost(url), ignore_www);
}

bool TLD::Host::operator==(const TLD::Host& other) const {
    return impl->fulldomain() == other.impl->fulldomain();
}

bool TLD::Host::operator==(const std::string& host) const {
    return impl->fulldomain() == host;
}
std::string_view TLD::Host::removeWWW(const std::string_view& host) noexcept {
    if (const size_t www_pos = host.find("www.");
        www_pos == std::string_view::npos || www_pos != 0) {
            return host;
    }
    return host.substr(4); // 4 is the length of the "www."
}
