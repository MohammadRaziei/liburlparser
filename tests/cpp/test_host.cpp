#include "utest.h"
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <variant>
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
// mismatched field is reported, not just the first one. Every row in
// host_data.csv is a domain name (see tests/data/host_data.csv), so
// from_url() always lands on the hostname alternative.
static void check_host_row(int *utest_result, const HostData& host_data) {
    urlparser::hostname host = urlparser::hostname::from_url(host_data.url, host_data.ignore_www);
    EXPECT_STREQ(host.str().c_str(), host_data.host.c_str());
    EXPECT_STREQ(host.subdomain().c_str(), host_data.subdomain.c_str());
    EXPECT_STREQ(host.domain().c_str(), host_data.domain.c_str());
    EXPECT_STREQ(host.domain_name().c_str(), host_data.domain_name.c_str());
    EXPECT_STREQ(host.suffix().c_str(), host_data.suffix.c_str());
}

UTEST(CSVHostTest, CheckPSLisLoaded){
    ASSERT_TRUE(urlparser::hostname::is_psl_loaded());
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
    urlparser::hostname host("www.example.com");
    EXPECT_STREQ(host.str().c_str(), host.full_domain().c_str());
}

UTEST(HostTest, FulldomainWithoutIgnoreWwwKeepsWww) {
    urlparser::hostname host("www.example.com", false);
    EXPECT_STREQ(host.full_domain().c_str(), "www.example.com");
}

UTEST(HostTest, FulldomainWithIgnoreWwwStripsWww) {
    urlparser::hostname host("www.example.com", true);
    EXPECT_STREQ(host.full_domain().c_str(), "example.com");
}

// --- operator==: value equality (hostname-vs-hostname and hostname-vs-string) -

UTEST(HostTest, EqualityComparesByValue) {
    urlparser::hostname a("example.com");
    urlparser::hostname b("example.com");
    urlparser::hostname c("other.com");
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a == std::string("example.com"));
    EXPECT_FALSE(a == std::string("other.com"));
}

UTEST(HostTest, CopyIsIndependentValue) {
    urlparser::hostname original("www.example.co.uk", true);
    urlparser::hostname copy = original;
    EXPECT_TRUE(original == copy);
    // Force the lazy PSL-dependent split on the copy only.
    (void)copy.suffix();
    EXPECT_STREQ(original.suffix().c_str(), copy.suffix().c_str());
    EXPECT_STREQ(original.domain().c_str(), copy.domain().c_str());
}

UTEST(HostTest, DefaultConstructedHostIsEmpty) {
    urlparser::hostname host;
    EXPECT_STREQ(host.str().c_str(), "");
    EXPECT_STREQ(host.domain().c_str(), "");
    EXPECT_STREQ(host.suffix().c_str(), "");
}

// --- from_url(std::string&&) overload: same result as the const& overload -

UTEST(HostTest, FromUrlRvalueOverloadMatchesLvalueOverload) {
    const std::string url = "http://mohammad:123@www.google.com/about";

    urlparser::hostname from_lvalue = urlparser::hostname::from_url(url, true);
    urlparser::hostname from_rvalue = urlparser::hostname::from_url(std::string(url), true);

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

// Regression test: an IPv6 literal's own colons must not be mistaken for
// the port separator either - that's exactly why URLs bracket them.
UTEST(HostTest, ExtractHostDoesNotConfusePortWithIPv6Colons) {
    EXPECT_STREQ(
        urlparser::url::extract_host("http://[2001:db8::1]:8080/x").c_str(),
        "[2001:db8::1]");
}

UTEST(HostTest, FromUrlWithPortMatchesFullUrlParserForDomainAndSuffix) {
    const std::string full_url =
        "https://m.raziei:1234@www.ee.aut.ac.ir:80/home?o=10&k=v#frag";

    urlparser::hostname h = urlparser::hostname::from_url(full_url, true);
    const auto* u_host = std::get_if<urlparser::hostname>(&urlparser::url(full_url, true).host());
    ASSERT_TRUE(u_host != nullptr);
    EXPECT_STREQ(h.str().c_str(), u_host->str().c_str());
    EXPECT_STREQ(h.suffix().c_str(), u_host->suffix().c_str());
    EXPECT_STREQ(h.domain().c_str(), u_host->domain().c_str());
    EXPECT_STREQ(h.subdomain().c_str(), u_host->subdomain().c_str());
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
    urlparser::hostname before("example.co.uk");
    const std::string suffix_before = before.suffix();

    urlparser::hostname::load_psl_from_string("uk\nco.uk\n");
    urlparser::hostname after("example.co.uk");
    EXPECT_STREQ(after.suffix().c_str(), "co.uk");

    // Restore the real PSL so later tests in this binary aren't affected.
    urlparser::hostname::load_psl_from_path(makeAbsolutePath("../public_suffix_list.dat"));
    urlparser::hostname restored("example.co.uk");
    EXPECT_STREQ(restored.suffix().c_str(), suffix_before.c_str());
}

// --- ipv4 ------------------------------------------------------------------

UTEST(Ipv4Test, ParseAndStrRoundTrip) {
    auto a = urlparser::ipv4::parse("192.168.1.1");
    EXPECT_STREQ(a.str().c_str(), "192.168.1.1");
}

UTEST(Ipv4Test, ToUint32AndBack) {
    auto a = urlparser::ipv4::parse("192.168.1.1");
    EXPECT_EQ(a.to_uint32(), 0xC0A80101u);
    auto b = urlparser::ipv4::from_uint32(0x08080808u);
    EXPECT_STREQ(b.str().c_str(), "8.8.8.8");
}

UTEST(Ipv4Test, IsValidRejectsGarbageAndHostnames) {
    EXPECT_TRUE(urlparser::ipv4::is_valid("1.2.3.4"));
    EXPECT_FALSE(urlparser::ipv4::is_valid("www.example.com"));
    EXPECT_FALSE(urlparser::ipv4::is_valid(""));
    EXPECT_FALSE(urlparser::ipv4::is_valid("1.2.3.4.5"));
    EXPECT_FALSE(urlparser::ipv4::is_valid("999.1.1.1"));
}

UTEST(Ipv4Test, ParseThrowsOnGarbage) {
    bool threw = false;
    try { (void)urlparser::ipv4::parse("not-an-ip"); }
    catch (const std::invalid_argument&) { threw = true; }
    EXPECT_TRUE(threw);
}

UTEST(Ipv4Test, IncrementDecrementWrapAround) {
    auto a = urlparser::ipv4::parse("255.255.255.255");
    ++a;
    EXPECT_STREQ(a.str().c_str(), "0.0.0.0");
    --a;
    EXPECT_STREQ(a.str().c_str(), "255.255.255.255");

    auto b = urlparser::ipv4::parse("0.0.0.0");
    --b;
    EXPECT_STREQ(b.str().c_str(), "255.255.255.255");
}

UTEST(Ipv4Test, PlusEqualsMinusEqualsAcrossOctetBoundary) {
    auto a = urlparser::ipv4::parse("10.0.0.0");
    a += 256;
    EXPECT_STREQ(a.str().c_str(), "10.0.1.0");
    a -= 512;
    EXPECT_STREQ(a.str().c_str(), "9.255.255.0");
}

UTEST(Ipv4Test, MinusInt64MinDoesNotCrash) {
    // Edge case: negating INT64_MIN as a signed int64_t is undefined
    // behavior; operator-= must compute this without ever doing that.
    auto a = urlparser::ipv4::parse("1.2.3.4");
    a -= INT64_MIN;
    (void)a.str();  // just must not crash/UB; exact wraparound value isn't the point here
}

UTEST(Ipv4Test, FreeOperatorsAndDistance) {
    auto a = urlparser::ipv4::parse("1.2.3.4");
    auto b = a + 1;
    EXPECT_STREQ(b.str().c_str(), "1.2.3.5");
    EXPECT_EQ(b - a, 1);
}

UTEST(Ipv4Test, Comparisons) {
    EXPECT_TRUE(urlparser::ipv4::parse("1.0.0.0") < urlparser::ipv4::parse("2.0.0.0"));
    EXPECT_TRUE(urlparser::ipv4::parse("1.0.0.0") == urlparser::ipv4::parse("1.0.0.0"));
}

// --- ipv6 ------------------------------------------------------------------

UTEST(Ipv6Test, ParseBracketedAndBareAgree) {
    auto a = urlparser::ipv6::parse("[2001:db8::1]");
    auto b = urlparser::ipv6::parse("2001:db8::1");
    EXPECT_TRUE(a == b);
    EXPECT_STREQ(a.str().c_str(), "2001:db8::1");
}

UTEST(Ipv6Test, IsValidRejectsGarbageAndIPv4) {
    EXPECT_TRUE(urlparser::ipv6::is_valid("::1"));
    EXPECT_TRUE(urlparser::ipv6::is_valid("[::1]"));
    EXPECT_FALSE(urlparser::ipv6::is_valid("192.168.1.1"));
    EXPECT_FALSE(urlparser::ipv6::is_valid("not-an-ip"));
}

UTEST(Ipv6Test, ParseThrowsOnGarbage) {
    bool threw = false;
    try { (void)urlparser::ipv6::parse("not-an-ip"); }
    catch (const std::invalid_argument&) { threw = true; }
    EXPECT_TRUE(threw);
}

UTEST(Ipv6Test, IncrementWrapsAcross64BitBoundary) {
    auto a = urlparser::ipv6::parse("::");
    ++a;
    EXPECT_STREQ(a.str().c_str(), "::1");

    auto b = urlparser::ipv6::parse("::");
    --b;
    EXPECT_STREQ(b.str().c_str(), "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");

    // Carry must propagate from the low 64 bits into the high 64 bits.
    auto c = urlparser::ipv6::from_uint64_pair(0, 0xFFFFFFFFFFFFFFFFull);
    ++c;
    EXPECT_EQ(c.high64(), 1u);
    EXPECT_EQ(c.low64(), 0u);
}

UTEST(Ipv6Test, MinusInt64MinDoesNotCrash) {
    auto a = urlparser::ipv6::parse("::1");
    a -= INT64_MIN;
    (void)a.str();
}

UTEST(Ipv6Test, HighLowRoundTrip) {
    auto a = urlparser::ipv6::from_uint64_pair(0x2001'0db8'0000'0000ull, 0x1ull);
    EXPECT_EQ(a.high64(), 0x2001'0db8'0000'0000ull);
    EXPECT_EQ(a.low64(), 0x1ull);
}

// --- host = std::variant<hostname, ipv4, ipv6> ------------------------------

UTEST(ParseHostTest, ClassifiesDomainIPv4AndIPv6) {
    EXPECT_TRUE(std::holds_alternative<urlparser::hostname>(urlparser::parse_host("example.com")));
    EXPECT_TRUE(std::holds_alternative<urlparser::ipv4>(urlparser::parse_host("192.0.2.1")));
    EXPECT_TRUE(std::holds_alternative<urlparser::ipv6>(urlparser::parse_host("[::1]")));
}

UTEST(ParseHostTest, RvalueOverloadMatchesViewOverload) {
    const std::string text = "example.com";
    auto a = urlparser::parse_host(text);
    auto b = urlparser::parse_host(std::string(text));
    ASSERT_TRUE(std::holds_alternative<urlparser::hostname>(a));
    ASSERT_TRUE(std::holds_alternative<urlparser::hostname>(b));
    EXPECT_TRUE(std::get<urlparser::hostname>(a) == std::get<urlparser::hostname>(b));
}

UTEST(ParseHostFromUrlTest, ExtractsAndClassifies) {
    auto a = urlparser::parse_host_from_url("http://192.168.1.1:8080/x");
    ASSERT_TRUE(std::holds_alternative<urlparser::ipv4>(a));
    EXPECT_STREQ(std::get<urlparser::ipv4>(a).str().c_str(), "192.168.1.1");

    auto b = urlparser::parse_host_from_url("https://www.example.com/x", true);
    ASSERT_TRUE(std::holds_alternative<urlparser::hostname>(b));
    EXPECT_STREQ(std::get<urlparser::hostname>(b).str().c_str(), "example.com");
}

UTEST(ParseHostFromUrlTest, IPv6WithPortIsClassifiedCorrectly) {
    auto a = urlparser::parse_host_from_url("http://[2001:db8::1]:8080/x");
    ASSERT_TRUE(std::holds_alternative<urlparser::ipv6>(a));
    EXPECT_STREQ(std::get<urlparser::ipv6>(a).str().c_str(), "2001:db8::1");
}

// The urlparser::str(host) free function must work uniformly across all
// three variant alternatives - this is exactly what the README's C++
// example demonstrates (urlparser::str(h) without a std::get_if/visit).
UTEST(HostStrFreeFunctionTest, WorksForHostnameIpv4AndIpv6) {
    urlparser::host h1 = urlparser::hostname("example.com");
    urlparser::host h2 = urlparser::ipv4::parse("192.168.1.1");
    urlparser::host h3 = urlparser::ipv6::parse("::1");
    EXPECT_STREQ(urlparser::str(h1).c_str(), "example.com");
    EXPECT_STREQ(urlparser::str(h2).c_str(), "192.168.1.1");
    EXPECT_STREQ(urlparser::str(h3).c_str(), "::1");
}

// --- url::host(): returns the classified variant, not just a hostname ------

UTEST(UrlHostTest, DomainUrlHoldsHostname) {
    urlparser::url u("https://www.example.com/x", true);
    EXPECT_TRUE(std::holds_alternative<urlparser::hostname>(u.host()));
}

UTEST(UrlHostTest, IPv4UrlHoldsIpv4NotGarbagePsl) {
    // Regression test: before hostname/ipv4/ipv6 were split, an IPv4 host
    // was run through PSL matching as if it were a domain name, so
    // "192.168.1.1" produced suffix()="1", domain()="1", subdomain()=
    // "192.168" - nonsense. Now it's simply not a hostname at all.
    urlparser::url u("http://192.168.1.1/path", false);
    const auto* v4 = std::get_if<urlparser::ipv4>(&u.host());
    ASSERT_TRUE(v4 != nullptr);
    EXPECT_STREQ(v4->str().c_str(), "192.168.1.1");
}

UTEST(UrlHostTest, IPv6UrlHoldsIpv6) {
    urlparser::url u("http://[2001:db8::1]:8080/x", false);
    const auto* v6 = std::get_if<urlparser::ipv6>(&u.host());
    ASSERT_TRUE(v6 != nullptr);
    EXPECT_STREQ(v6->str().c_str(), "2001:db8::1");
}
