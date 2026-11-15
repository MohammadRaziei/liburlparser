#include "utest.h"
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "urlparser.h"
#include "conftest.h"


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
    EXPECT_STREQ(url.host().str().c_str(), url_data.full_domain.c_str());
    const auto* h = url.host().try_hostname();
    ASSERT_TRUE(h != nullptr);
    EXPECT_STREQ(h->subdomain().c_str(), url_data.subdomain.c_str());
    EXPECT_STREQ(h->domain().c_str(), url_data.domain.c_str());
    EXPECT_STREQ(h->domain_name().c_str(), url_data.domain_name.c_str());
    EXPECT_STREQ(h->suffix().c_str(), url_data.suffix.c_str());
    EXPECT_EQ(url.port(), url_data.port);
    EXPECT_STREQ(std::string(url.query()).c_str(), url_data.query.c_str());
    EXPECT_STREQ(std::string(url.fragment()).c_str(), url_data.fragment.c_str());
}

#ifndef URLPARSER_TEST_NO_PSL  // these need the embedded Public Suffix List (URLPARSER_PSL=ON)
UTEST(CSVUrlTest, CheckPSLisLoaded){
    ASSERT_TRUE(urlparser::psl::instance().is_loaded());
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
#endif  // URLPARSER_TEST_NO_PSL

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

UTEST(UrlTest, AbspathDropsTrailingSlashOnNonRootPath) {
    // Regression test: path_is_already_normalized()'s fast path originally
    // only checked for '//' and '.'/'..' segments, missing that a bare
    // trailing '/' on a path with NEITHER of those also gets dropped by
    // the segment-by-segment resolver (the empty final segment after the
    // last '/' is skipped) - so a path like "/a/" was wrongly treated as
    // "already normalized" and returned unchanged instead of as "/a".
    // Found by tests/cpp/test_abspath_property.cpp against an independent
    // reference implementation, not by manual inspection - kept here as
    // the specific case rather than only in the fuzzer, so a plain
    // `ctest` run catches it without a separate fuzz invocation.
    urlparser::url url("https://example.com/a/");
    EXPECT_STREQ(url.abspath().c_str(), "/a");
}

UTEST(UrlTest, AbspathKeepsBareRootSlash) {
    // The trailing-slash trim above must not eat the root itself.
    urlparser::url url("https://example.com/");
    EXPECT_STREQ(url.abspath().c_str(), "/");
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

// --- query_params: the named type itself, and its operator<< -------------
// Previously untested (only ever used via `auto`), which hid a real bug:
// operator<< used literal '\b' (backspace) bytes to visually erase the
// trailing ", " separator - that only works on an interactive terminal;
// captured to a stream/string/file (exactly what this test does), the
// backspace bytes leaked into the output verbatim instead of erasing
// anything.

UTEST(UrlTest, QueryParamsIsUsableAsANamedType) {
    urlparser::url url("https://example.com/?a=1&b=2");
    urlparser::query_params params = url.params();
    ASSERT_EQ(params.size(), (size_t)2);
    EXPECT_STREQ(params[0].c_str(), "a=1");
}

UTEST(UrlTest, QueryParamsStreamOperatorFormatsCorrectly) {
    urlparser::query_params params = {"a=1", "b=2", "c=3"};
    std::ostringstream oss;
    oss << params;
    EXPECT_STREQ(oss.str().c_str(), "[a=1, b=2, c=3]");
}

UTEST(UrlTest, QueryParamsStreamOperatorHandlesEmpty) {
    urlparser::query_params params;
    std::ostringstream oss;
    oss << params;
    EXPECT_STREQ(oss.str().c_str(), "[]");
}

UTEST(UrlTest, QueryParamsStreamOperatorHandlesSingleElement) {
    urlparser::query_params params = {"only=1"};
    std::ostringstream oss;
    oss << params;
    EXPECT_STREQ(oss.str().c_str(), "[only=1]");
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
    const auto* copy_h = copy.host().try_hostname();
    const auto* original_h = original.host().try_hostname();
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

// --- default_port() / port_or_default() ---------------------------------

UTEST(UrlTest, DefaultPortCoversTheWhatwgSpecialSchemes) {
    EXPECT_EQ(urlparser::url::default_port("http"), 80);
    EXPECT_EQ(urlparser::url::default_port("https"), 443);
    EXPECT_EQ(urlparser::url::default_port("ws"), 80);
    EXPECT_EQ(urlparser::url::default_port("wss"), 443);
    EXPECT_EQ(urlparser::url::default_port("ftp"), 21);
}

UTEST(UrlTest, DefaultPortIsZeroForSchemesWithoutOne) {
    EXPECT_EQ(urlparser::url::default_port("ssh"), 0);
    EXPECT_EQ(urlparser::url::default_port("socks5"), 0);
    EXPECT_EQ(urlparser::url::default_port("gopher"), 0);
    EXPECT_EQ(urlparser::url::default_port(""), 0);
    // Matched as given (protocol() already returns lowercase), not case-folded.
    EXPECT_EQ(urlparser::url::default_port("HTTP"), 0);
}

UTEST(UrlTest, PortOrDefaultPrefersTheExplicitPort) {
    EXPECT_EQ(urlparser::url("https://example.com:8443/").port_or_default(), 8443);
    EXPECT_EQ(urlparser::url("http://example.com:443/").port_or_default(), 443);
}

UTEST(UrlTest, PortOrDefaultFallsBackToTheSchemeDefault) {
    urlparser::url plain("http://example.com/");
    EXPECT_EQ(plain.port(), 0);  // port() itself stays "not specified"
    EXPECT_EQ(plain.port_or_default(), 80);
    EXPECT_EQ(urlparser::url("https://example.com").port_or_default(), 443);
    EXPECT_EQ(urlparser::url("HTTPS://Example.com/").port_or_default(), 443);
    EXPECT_EQ(urlparser::url("example.com/path").port_or_default(), 0);
    EXPECT_EQ(urlparser::url("gopher://example.com/").port_or_default(), 0);
}

// --- username() / password() --------------------------------------------

UTEST(UrlTest, UsernameAndPasswordSplitUserinfoAtTheFirstColon) {
    urlparser::url url("https://user:pass@example.com/");
    EXPECT_STREQ(std::string(url.username()).c_str(), "user");
    EXPECT_STREQ(std::string(url.password()).c_str(), "pass");
}

UTEST(UrlTest, PasswordKeepsLaterColons) {
    urlparser::url url("https://user:a:b@example.com/");
    EXPECT_STREQ(std::string(url.username()).c_str(), "user");
    EXPECT_STREQ(std::string(url.password()).c_str(), "a:b");
}

UTEST(UrlTest, UsernameWithoutColonHasEmptyPassword) {
    urlparser::url url("https://user@example.com/");
    EXPECT_STREQ(std::string(url.username()).c_str(), "user");
    EXPECT_TRUE(url.password().empty());
}

UTEST(UrlTest, EmptyPasswordAndEmptyUsernameAreBothAllowed) {
    urlparser::url no_password("https://user:@example.com/");
    EXPECT_STREQ(std::string(no_password.username()).c_str(), "user");
    EXPECT_TRUE(no_password.password().empty());

    urlparser::url no_user("https://:secret@example.com/");
    EXPECT_TRUE(no_user.username().empty());
    EXPECT_STREQ(std::string(no_user.password()).c_str(), "secret");
}

UTEST(UrlTest, NoUserinfoMeansEmptyUsernameAndPassword) {
    urlparser::url url("https://example.com/");
    EXPECT_TRUE(url.username().empty());
    EXPECT_TRUE(url.password().empty());
}

UTEST(UrlTest, UsernameAndPasswordStayPercentEncoded) {
    urlparser::url url("https://us%40er:p%3Aw@example.com/");
    EXPECT_STREQ(std::string(url.username()).c_str(), "us%40er");
    EXPECT_STREQ(std::string(url.password()).c_str(), "p%3Aw");
    EXPECT_STREQ(urlparser::percent_codec::decode(url.username()).c_str(), "us@er");
    EXPECT_STREQ(urlparser::percent_codec::decode(url.password()).c_str(), "p:w");
}

// --- IPv4 / IPv6 literal hosts ------------------------------------------

UTEST(UrlTest, Ipv4HostWithPort) {
    urlparser::url url("http://127.0.0.1:8080/x");
    EXPECT_STREQ(std::string(url.host_text()).c_str(), "127.0.0.1");
    EXPECT_EQ(url.port(), 8080);
    EXPECT_TRUE(std::get_if<urlparser::ipv4>(&url.host().variant()) != nullptr);
}

UTEST(UrlTest, Ipv6HostKeepsItsBracketsInHostText) {
    urlparser::url url("http://[::1]:8080/");
    EXPECT_STREQ(std::string(url.host_text()).c_str(), "[::1]");
    EXPECT_EQ(url.port(), 8080);
    EXPECT_STREQ(url.abspath().c_str(), "/");
    EXPECT_TRUE(std::get_if<urlparser::ipv6>(&url.host().variant()) != nullptr);
}

UTEST(UrlTest, Ipv6HostWithoutPort) {
    urlparser::url url("http://[::1]/path");
    EXPECT_STREQ(std::string(url.host_text()).c_str(), "[::1]");
    EXPECT_EQ(url.port(), 0);
    EXPECT_EQ(url.port_or_default(), 80);
    EXPECT_STREQ(std::string(url.path()).c_str(), "/path");
}

UTEST(UrlTest, Ipv6HostWithUserinfoAndQuery) {
    urlparser::url url("https://user:pw@[2001:db8::1]:8443/a?b=1");
    EXPECT_STREQ(std::string(url.username()).c_str(), "user");
    EXPECT_STREQ(std::string(url.password()).c_str(), "pw");
    EXPECT_STREQ(std::string(url.host_text()).c_str(), "[2001:db8::1]");
    EXPECT_EQ(url.port(), 8443);
    EXPECT_STREQ(std::string(url.query()).c_str(), "b=1");
}

UTEST(UrlTest, UnknownSchemeStillParses) {
    // The parser is scheme-agnostic: deciding which schemes a caller accepts is the caller's policy.
    urlparser::url url("gopher://example.com:70/x");
    EXPECT_STREQ(std::string(url.protocol()).c_str(), "gopher");
    EXPECT_STREQ(std::string(url.host_text()).c_str(), "example.com");
    EXPECT_EQ(url.port(), 70);
}

// --- implicit construction from std::string / const char* ---------------

static std::string describe_url(const urlparser::url& u) {
    return std::string(u.protocol()) + "|" + std::string(u.host_text()) + "|" + std::to_string(u.port_or_default());
}

UTEST(UrlTest, ConvertsImplicitlyFromStringLiteralAndStdString) {
    EXPECT_STREQ(describe_url("https://example.com/a").c_str(), "https|example.com|443");
    const std::string text = "http://[::1]:8080/";
    EXPECT_STREQ(describe_url(text).c_str(), "http|[::1]|8080");
    EXPECT_STREQ(describe_url(std::string_view("http://example.com")).c_str(), "http|example.com|80");
    EXPECT_STREQ(describe_url(static_cast<const char*>(nullptr)).c_str(), "||0");
}
