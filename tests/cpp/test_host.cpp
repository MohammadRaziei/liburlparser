#include "utest.h"
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "urlparser.h"
#include "common.h"



struct HostData : public BaseData{
    std::string url;
    bool ignore_www;
    std::string host;
    std::string subdomain;
    std::string domain;
    std::string domain_name;
    std::string suffix;

    std::string toString() const{
        std::stringstream ss;
        ss << std::boolalpha;
        ss << "{" << "url: " << url << ", ignore_www: " << ignore_www
            << ", host: " << host << ", subdomain: " << subdomain
            << ", domain: " << domain << ", domain_name: " << domain_name
            << ", suffix: " << suffix << "}";
        return ss.str();
    }
};


std::vector<HostData> load_host_data(const std::string& filename) {
    std::vector<HostData> host_data_list;
    std::ifstream f(filename);
    std::string line;
    std::getline(f, line); // Skip the header line
    while (std::getline(f, line)) {
        std::istringstream ss(line);
        HostData host_data;
        std::string ignore_www;
        std::getline(ss, host_data.url, ',');
        std::getline(ss, ignore_www, ',');
        std::getline(ss, host_data.host, ',');
        std::getline(ss, host_data.subdomain, ',');
        std::getline(ss, host_data.domain, ',');
        std::getline(ss, host_data.domain_name, ',');
        std::getline(ss, host_data.suffix);
        host_data.ignore_www = ignore_www == "True";
        host_data_list.push_back(host_data);
    }
    return host_data_list;
}


// Runs all field checks for a single CSV row. Non-fatal (EXPECT_*) so every
// mismatched field is reported, not just the first one.
static void check_host_row(int *utest_result, const HostData& host_data) {
    urlparser::host host = urlparser::host::from_url(host_data.url, host_data.ignore_www);
    EXPECT_STREQ(host.str().c_str(), host_data.host.c_str());
    EXPECT_STREQ(host.subdomain().c_str(), host_data.subdomain.c_str());
    EXPECT_STREQ(host.domain().c_str(), host_data.domain.c_str());
    EXPECT_STREQ(host.domain_name().c_str(), host_data.domain_name.c_str());
    EXPECT_STREQ(host.suffix().c_str(), host_data.suffix.c_str());
}

UTEST(CSVHostTest, CheckPSLisLoaded){
    ASSERT_TRUE(urlparser::host::is_psl_loaded());
}

UTEST(CSVHostTest, HostDataInput) {
    const std::vector<HostData> rows = load_host_data(makeAbsolutePath("data/host_data.csv"));
    for (size_t i = 0; i < rows.size(); ++i) {
        const HostData& row = rows[i];
        int row_result = 0;
        check_host_row(&row_result, row);
        if (row_result) {
            UTEST_PRINTF("  ^ failed on row %zu: %s\n", i, row.toString().c_str());
            *utest_result = 1;
        }
    }
}

// --- str()/full_domain(): the PSL-free fast path -------------------------

UTEST(HostTest, StrEqualsFulldomain) {
    urlparser::host host("www.example.com");
    EXPECT_STREQ(host.str().c_str(), host.full_domain().c_str());
}

UTEST(HostTest, FulldomainWithoutIgnoreWwwKeepsWww) {
    urlparser::host host("www.example.com", false);
    EXPECT_STREQ(host.full_domain().c_str(), "www.example.com");
}

UTEST(HostTest, FulldomainWithIgnoreWwwStripsWww) {
    urlparser::host host("www.example.com", true);
    EXPECT_STREQ(host.full_domain().c_str(), "example.com");
}

// --- operator==: value equality (host-vs-host and host-vs-string) -------

UTEST(HostTest, EqualityComparesByValue) {
    urlparser::host a("example.com");
    urlparser::host b("example.com");
    urlparser::host c("other.com");
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a == std::string("example.com"));
    EXPECT_FALSE(a == std::string("other.com"));
}

UTEST(HostTest, CopyIsIndependentValue) {
    urlparser::host original("www.example.co.uk", true);
    urlparser::host copy = original;
    EXPECT_TRUE(original == copy);
    // Force the lazy PSL-dependent split on the copy only.
    (void)copy.suffix();
    EXPECT_STREQ(original.suffix().c_str(), copy.suffix().c_str());
    EXPECT_STREQ(original.domain().c_str(), copy.domain().c_str());
}

UTEST(HostTest, DefaultConstructedHostIsEmpty) {
    urlparser::host host;
    EXPECT_STREQ(host.str().c_str(), "");
    EXPECT_STREQ(host.domain().c_str(), "");
    EXPECT_STREQ(host.suffix().c_str(), "");
}

// --- from_url(std::string&&) overload: same result as the const& overload -

UTEST(HostTest, FromUrlRvalueOverloadMatchesLvalueOverload) {
    const std::string url = "http://mohammad:123@www.google.com/about";

    urlparser::host from_lvalue = urlparser::host::from_url(url, true);
    urlparser::host from_rvalue = urlparser::host::from_url(std::string(url), true);

    EXPECT_TRUE(from_lvalue == from_rvalue);
    EXPECT_STREQ(from_lvalue.str().c_str(), from_rvalue.str().c_str());
}

UTEST(HostTest, ExtractHostRvalueOverloadMatchesViewOverload) {
    const std::string url = "https://user@sub.example.co.uk:8080/path?q=1";

    std::string from_view = urlparser::url::extract_host(url);
    std::string from_rvalue = urlparser::url::extract_host(std::string(url));

    EXPECT_STREQ(from_view.c_str(), from_rvalue.c_str());
}

// --- extract_host()/from_url(): host boundary must agree with url::url() -

// Regression test: extract_host() used to find its end-of-host boundary
// with find_first_of("?/", pos), which doesn't know about ':' (port) or '#'
// (fragment) at all. So a URL with an explicit port had its port digits
// silently glued onto the returned host string, and everything downstream
// (suffix/domain/subdomain, computed from that polluted string) came out
// wrong too - all while url::url()'s own, separately-written parser handled
// the exact same input correctly. Both now share one scan_authority() so
// they can't drift apart like that again.
UTEST(HostTest, ExtractHostStripsPort) {
    EXPECT_STREQ(
        urlparser::url::extract_host("https://www.example.com:8080/path").c_str(),
        "www.example.com");
}

UTEST(HostTest, ExtractHostStopsAtFragmentWithNoPath) {
    // No '/' or '?' at all - only a '#' - previously fell through to
    // "end of string", swallowing the fragment into the host.
    EXPECT_STREQ(urlparser::url::extract_host("https://example.com#frag").c_str(),
                 "example.com");
}

UTEST(HostTest, ExtractHostDoesNotConfusePortWithUserinfoColon) {
    // The userinfo itself contains a ':' (user:pass@...); that must not be
    // mistaken for the port separator that comes later, after the '@'.
    EXPECT_STREQ(
        urlparser::url::extract_host("https://user:pass@host.example.com:80/x").c_str(),
        "host.example.com");
}

UTEST(HostTest, FromUrlWithPortMatchesFullUrlParserForDomainAndSuffix) {
    const std::string full_url =
        "https://m.raziei:1234@www.ee.aut.ac.ir:80/home?o=10&k=v#frag";

    urlparser::host h = urlparser::host::from_url(full_url, true);
    urlparser::url u(full_url, true);

    EXPECT_STREQ(h.str().c_str(), u.host().str().c_str());
    EXPECT_STREQ(h.suffix().c_str(), u.suffix().c_str());
    EXPECT_STREQ(h.domain().c_str(), u.domain().c_str());
    EXPECT_STREQ(h.subdomain().c_str(), u.subdomain().c_str());
}

// --- version class --------------------------------------------------------

UTEST(VersionTest, MatchesVersionMacros) {
    EXPECT_EQ(urlparser::version::major(), (unsigned int)URLPARSER_VERSION_MAJOR);
    EXPECT_EQ(urlparser::version::minor(), (unsigned int)URLPARSER_VERSION_MINOR);
    EXPECT_EQ(urlparser::version::patch(), (unsigned int)URLPARSER_VERSION_PATCH);
    EXPECT_STREQ(std::string(urlparser::version::string()).c_str(), URLPARSER_VERSION_STRING);
}

// --- PSL reload: load_psl_from_string should take effect immediately -------

UTEST(HostTest, LoadPslFromStringChangesFutureLookups) {
    // A minimal, deliberately different PSL: only "co.uk" (not "com") is a
    // registered multi-level suffix here, so "example.com" now resolves
    // with "com" as a plain single-level suffix (which it already was),
    // but "example.co.uk" must now resolve suffix="co.uk", domain="example" -
    // the same as the real PSL would give, just confirming a reload of the
    // *same* content round-trips correctly (we restore the real PSL right
    // after, so we don't leak this into other tests).
    urlparser::host before("example.co.uk");
    const std::string suffix_before = before.suffix();

    urlparser::host::load_psl_from_string("uk\nco.uk\n");
    urlparser::host after("example.co.uk");
    EXPECT_STREQ(after.suffix().c_str(), "co.uk");

    // Restore the real PSL so later tests in this binary aren't affected.
    urlparser::host::load_psl_from_path(makeAbsolutePath("../public_suffix_list.dat"));
    urlparser::host restored("example.co.uk");
    EXPECT_STREQ(restored.suffix().c_str(), suffix_before.c_str());
}
