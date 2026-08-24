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
    urlparser::Host host = urlparser::Host::fromUrl(host_data.url, host_data.ignore_www);
    EXPECT_STREQ(host.str().c_str(), host_data.host.c_str());
    EXPECT_STREQ(host.subdomain().c_str(), host_data.subdomain.c_str());
    EXPECT_STREQ(host.domain().c_str(), host_data.domain.c_str());
    EXPECT_STREQ(host.domainName().c_str(), host_data.domain_name.c_str());
    EXPECT_STREQ(host.suffix().c_str(), host_data.suffix.c_str());
}

UTEST(CSVHostTest, CheckPSLisLoaded){
    ASSERT_TRUE(urlparser::Host::isPslLoaded());
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

// --- str()/fulldomain(): the PSL-free fast path -------------------------

UTEST(HostTest, StrEqualsFulldomain) {
    urlparser::Host host("www.example.com");
    EXPECT_STREQ(host.str().c_str(), host.fulldomain().c_str());
}

UTEST(HostTest, FulldomainWithoutIgnoreWwwKeepsWww) {
    urlparser::Host host("www.example.com", false);
    EXPECT_STREQ(host.fulldomain().c_str(), "www.example.com");
}

UTEST(HostTest, FulldomainWithIgnoreWwwStripsWww) {
    urlparser::Host host("www.example.com", true);
    EXPECT_STREQ(host.fulldomain().c_str(), "example.com");
}

// --- operator==: value equality (Host-vs-Host and Host-vs-string) -------

UTEST(HostTest, EqualityComparesByValue) {
    urlparser::Host a("example.com");
    urlparser::Host b("example.com");
    urlparser::Host c("other.com");
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a == std::string("example.com"));
    EXPECT_FALSE(a == std::string("other.com"));
}

UTEST(HostTest, CopyIsIndependentValue) {
    urlparser::Host original("www.example.co.uk", true);
    urlparser::Host copy = original;
    EXPECT_TRUE(original == copy);
    // Force the lazy PSL-dependent split on the copy only.
    (void)copy.suffix();
    EXPECT_STREQ(original.suffix().c_str(), copy.suffix().c_str());
    EXPECT_STREQ(original.domain().c_str(), copy.domain().c_str());
}

UTEST(HostTest, DefaultConstructedHostIsEmpty) {
    urlparser::Host host;
    EXPECT_STREQ(host.str().c_str(), "");
    EXPECT_STREQ(host.domain().c_str(), "");
    EXPECT_STREQ(host.suffix().c_str(), "");
}

// --- version class --------------------------------------------------------

UTEST(VersionTest, MatchesVersionMacros) {
    EXPECT_EQ(urlparser::version::major(), (unsigned int)URLPARSER_VERSION_MAJOR);
    EXPECT_EQ(urlparser::version::minor(), (unsigned int)URLPARSER_VERSION_MINOR);
    EXPECT_EQ(urlparser::version::patch(), (unsigned int)URLPARSER_VERSION_PATCH);
    EXPECT_STREQ(std::string(urlparser::version::string()).c_str(), URLPARSER_VERSION_STRING);
}

// --- PSL reload: loadPslFromString should take effect immediately -------

UTEST(HostTest, LoadPslFromStringChangesFutureLookups) {
    // A minimal, deliberately different PSL: only "co.uk" (not "com") is a
    // registered multi-level suffix here, so "example.com" now resolves
    // with "com" as a plain single-level suffix (which it already was),
    // but "example.co.uk" must now resolve suffix="co.uk", domain="example" -
    // the same as the real PSL would give, just confirming a reload of the
    // *same* content round-trips correctly (we restore the real PSL right
    // after, so we don't leak this into other tests).
    urlparser::Host before("example.co.uk");
    const std::string suffix_before = before.suffix();

    urlparser::Host::loadPslFromString("uk\nco.uk\n");
    urlparser::Host after("example.co.uk");
    EXPECT_STREQ(after.suffix().c_str(), "co.uk");

    // Restore the real PSL so later tests in this binary aren't affected.
    urlparser::Host::loadPslFromPath(makeAbsolutePath("../public_suffix_list.dat"));
    urlparser::Host restored("example.co.uk");
    EXPECT_STREQ(restored.suffix().c_str(), suffix_before.c_str());
}
