#include "utest.h"
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "urlparser.h"
#include "common.h"


struct UrlData : public BaseData{
    std::string url;
    bool ignore_www;
    std::string protocol;
    std::string userinfo;
    std::string full_domain;
    std::string subdomain;
    std::string domain;
    std::string domain_name;
    std::string suffix;
    int port;
    std::string query;
    std::string fragment;

    std::string toString() const{
        std::stringstream ss;
        ss << std::boolalpha;
        ss << "{" << "url: " << url << ", ignore_www: " << ignore_www
           << ", url: " << url << ", subdomain: " << subdomain
           << ", domain: " << domain << ", domain_name: " << domain_name
           << ", suffix: " << suffix << "}";
        return ss.str();
    }
};


std::vector<UrlData> load_url_data(const std::string& filename) {
    std::vector<UrlData> url_data_list;
    std::ifstream f(filename);
    std::string line;
    std::getline(f, line); // Skip the header line
    while (std::getline(f, line)) {
        std::istringstream ss(line);
        UrlData url_data;
        std::string ignore_www, port;
        // url,ignore_www,protocol,userinfo,full_domain,subdomain,domain,domain_name,suffix,port,query,fragment

        std::getline(ss, url_data.url, ',');
        std::getline(ss, ignore_www, ',');
        url_data.ignore_www = ignore_www == "True";
        std::getline(ss, url_data.protocol, ',');
        std::getline(ss, url_data.userinfo, ',');
        std::getline(ss, url_data.full_domain, ',');
        std::getline(ss, url_data.subdomain, ',');
        std::getline(ss, url_data.domain, ',');
        std::getline(ss, url_data.domain_name, ',');
        std::getline(ss, url_data.suffix, ',');
        std::getline(ss, port, ',');
        url_data.port = std::stoi(port);
        std::getline(ss, url_data.query, ',');
        std::getline(ss, url_data.fragment, ',');

        url_data_list.push_back(url_data);
    }
    return url_data_list;
}


// Runs all field checks for a single CSV row. Non-fatal (EXPECT_*) so every
// mismatched field is reported, not just the first one. Every row in
// url_data.csv is a domain name, so url.host() always holds a hostname.
static void check_url_row(int *utest_result, const UrlData& url_data) {
    urlparser::url url(url_data.url, url_data.ignore_www);
    EXPECT_STREQ(std::string(url.protocol()).c_str(), url_data.protocol.c_str());
    EXPECT_STREQ(std::string(url.userinfo()).c_str(), url_data.userinfo.c_str());
    EXPECT_STREQ(std::string(url.host_text()).c_str(), url_data.full_domain.c_str());
    EXPECT_STREQ(urlparser::str(url.host()).c_str(), url_data.full_domain.c_str());
    const auto* h = std::get_if<urlparser::hostname>(&url.host());
    ASSERT_TRUE(h != nullptr);
    EXPECT_STREQ(h->subdomain().c_str(), url_data.subdomain.c_str());
    EXPECT_STREQ(h->domain().c_str(), url_data.domain.c_str());
    EXPECT_STREQ(h->domain_name().c_str(), url_data.domain_name.c_str());
    EXPECT_STREQ(h->suffix().c_str(), url_data.suffix.c_str());
    EXPECT_EQ(url.port(), url_data.port);
    EXPECT_STREQ(std::string(url.query()).c_str(), url_data.query.c_str());
    EXPECT_STREQ(std::string(url.fragment()).c_str(), url_data.fragment.c_str());
}

UTEST(CSVUrlTest, CheckPSLisLoaded){
    ASSERT_TRUE(urlparser::hostname::is_psl_loaded());
}

UTEST(CSVUrlTest, UrlDataInput) {
    const std::vector<UrlData> rows = load_url_data(makeAbsolutePath("data/url_data.csv"));
    for (size_t i = 0; i < rows.size(); ++i) {
        const UrlData& row = rows[i];
        int row_result = 0;
        check_url_row(&row_result, row);
        if (row_result) {
            UTEST_PRINTF("  ^ failed on row %zu: %s\n", i, row.toString().c_str());
            *utest_result = 1;
        }
    }
}

// --- str(): full URL reconstruction ------------------------------------

UTEST(UrlTest, StrRoundTripsSimpleUrl) {
    urlparser::url url("https://example.com/path?a=1#frag");
    EXPECT_STREQ(url.str().c_str(), "https://example.com/path?a=1#frag");
}

UTEST(UrlTest, StrIncludesPortWhenNonDefault) {
    urlparser::url url("http://example.com:8080/");
    EXPECT_STREQ(url.str().c_str(), "http://example.com:8080/");
    EXPECT_EQ(url.port(), 8080);
}

UTEST(UrlTest, StrIncludesUserinfo) {
    urlparser::url url("https://user:pass@example.com/");
    EXPECT_STREQ(std::string(url.userinfo()).c_str(), "user:pass");
    EXPECT_STREQ(url.str().c_str(), "https://user:pass@example.com/");
}

UTEST(UrlTest, StrOmitsEmptyPathAsRoot) {
    urlparser::url url("https://example.com");
    // No explicit path in the input, but there IS an authority -> "/" is
    // synthesized on reconstruction (matches str()'s documented behavior).
    EXPECT_STREQ(url.str().c_str(), "https://example.com/");
}

// --- abspath(): '.'/'..' segment resolution, pure (no side effects) ----

UTEST(UrlTest, AbspathResolvesDotDot) {
    urlparser::url url("https://example.com/a/b/../c");
    EXPECT_STREQ(url.abspath().c_str(), "/a/c");
}

UTEST(UrlTest, AbspathResolvesDot) {
    urlparser::url url("https://example.com/a/./b/./c");
    EXPECT_STREQ(url.abspath().c_str(), "/a/b/c");
}

UTEST(UrlTest, AbspathCollapsesRepeatedSlashes) {
    urlparser::url url("https://example.com/a//b///c");
    EXPECT_STREQ(url.abspath().c_str(), "/a/b/c");
}

UTEST(UrlTest, AbspathIsPureNotMutating) {
    // Calling abspath() must not change path()/str() as a side effect -
    // regression test for a bug in the old chainable UrlImpl::abspath(),
    // which mutated path_ in place.
    urlparser::url url("https://example.com/a/../b");
    const std::string before = url.str();
    (void)url.abspath();
    (void)url.abspath();  // call twice to be sure
    EXPECT_STREQ(url.str().c_str(), before.c_str());
}

// --- params(): '&'-separated query splitting ----------------------------

UTEST(UrlTest, ParamsSplitsOnAmpersand) {
    urlparser::url url("https://example.com/?a=1&b=2&c=3");
    const auto params = url.params();
    ASSERT_EQ(params.size(), (size_t)3);
    EXPECT_STREQ(params[0].c_str(), "a=1");
    EXPECT_STREQ(params[1].c_str(), "b=2");
    EXPECT_STREQ(params[2].c_str(), "c=3");
}

UTEST(UrlTest, ParamsEmptyWhenNoQuery) {
    urlparser::url url("https://example.com/");
    EXPECT_TRUE(url.params().empty());
}

// --- operator==: value equality, not identity ---------------------------

UTEST(UrlTest, EqualityComparesByValue) {
    urlparser::url a("https://example.com/path?x=1");
    urlparser::url b("https://example.com/path?x=1");
    urlparser::url c("https://example.com/other");
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

UTEST(UrlTest, CopyIsIndependentValue) {
    // Since PIMPL/shared_ptr sharing was removed, a copy must be a fully
    // independent value - mutating access patterns on one must never affect
    // the other's observable state.
    urlparser::url original("https://example.com/a/b");
    urlparser::url copy = original;
    EXPECT_TRUE(original == copy);
    EXPECT_STREQ(copy.str().c_str(), original.str().c_str());
    // Force lazy host construction on the copy only, then confirm both
    // still report identical, correct results.
    const auto* copy_h = std::get_if<urlparser::hostname>(&copy.host());
    const auto* original_h = std::get_if<urlparser::hostname>(&original.host());
    ASSERT_TRUE(copy_h != nullptr);
    ASSERT_TRUE(original_h != nullptr);
    EXPECT_STREQ(original_h->domain().c_str(), copy_h->domain().c_str());
}

UTEST(UrlTest, DefaultConstructedUrlIsEmpty) {
    urlparser::url url;
    EXPECT_STREQ(std::string(url.protocol()).c_str(), "");
    EXPECT_STREQ(std::string(url.host_text()).c_str(), "");
    EXPECT_EQ(url.port(), 0);
}

// --- malformed ports: exercises the std::from_chars-based parser --------

UTEST(UrlTest, NonNumericPortThrows) {
    bool threw = false;
    try {
        urlparser::url url("http://example.com:abc/");
        (void)url;
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}

UTEST(UrlTest, PortAboveMaxThrows) {
    bool threw = false;
    try {
        urlparser::url url("http://example.com:99999/");
        (void)url;
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}

UTEST(UrlTest, EmptyPortDefaultsToZero) {
    urlparser::url url("http://example.com:/path");
    EXPECT_EQ(url.port(), 0);
}
