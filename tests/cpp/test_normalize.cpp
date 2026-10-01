#include "utest.h"
#include <string>
#include <utility>
#include <vector>

#include "urlparser.h"

// Gap: liburlparser already lowercases scheme/host at parse time, but had
// no equivalent of ada's normalize_url() for the rest: str() keeps the raw
// (non-dot-resolved) path, keeps an explicit default port ("https://h:443"
// stays ":443"), doesn't IDNA-normalize a Unicode host, and the parser
// didn't trim leading/trailing whitespace at all. Expected values verified
// against ada_url.normalize_url() where applicable.

UTEST(UrlTest, NormalizedResolvesDotSegments) {
    urlparser::url u("https://example.com/a/../b");
    EXPECT_STREQ(u.normalized().c_str(), "https://example.com/b");
    // str() is unaffected - still the raw, unresolved path.
    EXPECT_STREQ(u.str().c_str(), "https://example.com/a/../b");
}

UTEST(UrlTest, NormalizedOmitsDefaultPortForScheme) {
    urlparser::url https_default("https://example.com:443/path");
    EXPECT_STREQ(https_default.normalized().c_str(), "https://example.com/path");

    urlparser::url http_default("http://example.com:80/path");
    EXPECT_STREQ(http_default.normalized().c_str(), "http://example.com/path");
}

UTEST(UrlTest, NormalizedKeepsNonDefaultPort) {
    urlparser::url u("https://example.com:8443/path");
    EXPECT_STREQ(u.normalized().c_str(), "https://example.com:8443/path");
}

UTEST(UrlTest, NormalizedKeepsPortForNonSpecialScheme) {
    // "ssh" isn't a WHATWG special scheme - it has no "default port" to
    // omit, so an explicit port is always kept.
    urlparser::url u("ssh://git@example.com:22/repo.git");
    EXPECT_STREQ(u.normalized().c_str(), "ssh://git@example.com:22/repo.git");
}

UTEST(UrlTest, NormalizedAppliesIdnaToUnicodeHost) {
    urlparser::url u("https://caf\xc3\xa9.com/path");
    EXPECT_STREQ(u.normalized().c_str(), "https://xn--caf-dma.com/path");
}

UTEST(UrlTest, NormalizedGivesEmptyPathARoot) {
    urlparser::url u("https://example.com");
    EXPECT_STREQ(u.normalized().c_str(), "https://example.com/");
}

UTEST(UrlTest, NormalizedKeepsQueryAndFragmentAsGiven) {
    urlparser::url u("https://example.com/path?b=2&a=1#frag");
    EXPECT_STREQ(u.normalized().c_str(), "https://example.com/path?b=2&a=1#frag");
}

UTEST(UrlTest, WsAndWssSerializeWithDoubleSlash) {
    // Regression: ws/wss were missing from the netloc-scheme allow-list,
    // so "wss://host/path" was serializing back out as "wss:host/path"
    // (missing "//") - found by comparing normalized() against
    // ada_url.normalize_url() on a broader case set.
    EXPECT_STREQ(urlparser::url("ws://example.com/socket").str().c_str(),
                 "ws://example.com/socket");
    EXPECT_STREQ(urlparser::url("wss://example.com/socket").str().c_str(),
                 "wss://example.com/socket");
}

// --- get_search_params(): decoded key -> values map --------------------
// Gap: params() only ever gave the raw, still percent-encoded "key=value"
// strings; there was no built-in way to get "the value of query
// parameter x" without manually splitting and percent-decoding yourself.

UTEST(UrlTest, SearchParamsDecodesSimpleKeyValues) {
    urlparser::url u("https://example.com/?a=1&b=2");
    auto sp = u.get_search_params();
    ASSERT_EQ(sp.size(), (size_t)2);
    ASSERT_EQ(sp.at("a").size(), (size_t)1);
    EXPECT_STREQ(sp.at("a")[0].c_str(), "1");
    EXPECT_STREQ(sp.at("b")[0].c_str(), "2");
}

UTEST(UrlTest, SearchParamsCollectsRepeatedKeys) {
    urlparser::url u("https://example.com/?a=1&a=3&b=2");
    auto sp = u.get_search_params();
    ASSERT_EQ(sp.size(), (size_t)2);
    ASSERT_EQ(sp.at("a").size(), (size_t)2);
    EXPECT_STREQ(sp.at("a")[0].c_str(), "1");
    EXPECT_STREQ(sp.at("a")[1].c_str(), "3");
}

UTEST(UrlTest, SearchParamsPercentDecodesKeysAndValues) {
    urlparser::url u("https://example.com/?name=caf%C3%A9&q=a%20b");
    auto sp = u.get_search_params();
    EXPECT_STREQ(sp.at("name")[0].c_str(), "caf\xc3\xa9");
    EXPECT_STREQ(sp.at("q")[0].c_str(), "a b");
}

UTEST(UrlTest, SearchParamsHandlesValuelessKey) {
    // "flag" with no '=' at all -> empty-string value, still present.
    urlparser::url u("https://example.com/?flag&a=1");
    auto sp = u.get_search_params();
    ASSERT_TRUE(sp.count("flag") == 1);
    EXPECT_STREQ(sp.at("flag")[0].c_str(), "");
}

UTEST(UrlTest, SearchParamsEmptyWhenNoQuery) {
    urlparser::url u("https://example.com/");
    EXPECT_TRUE(u.get_search_params().empty());
}

// Found by thinking about what my randomized comparison against ada could
// NOT have caught: its generator built inputs with urllib's quote(), which
// writes '+' as %2B, so a *literal* '+' never appeared in any test input.

UTEST(UrlTest, SearchParamsPlusIsSpaceInValue) {
    urlparser::url u("https://example.com/?q=hello+world");
    EXPECT_STREQ(u.get_search_params().at("q")[0].c_str(), "hello world");
}

UTEST(UrlTest, SearchParamsPlusIsSpaceInKey) {
    urlparser::url u("https://example.com/?a+b=1");
    auto sp = u.get_search_params();
    ASSERT_TRUE(sp.count("a b") == 1);
    EXPECT_STREQ(sp.at("a b")[0].c_str(), "1");
}

UTEST(UrlTest, SearchParamsEncodedPlusStaysAPlus) {
    urlparser::url u("https://example.com/?a=%2B&b=1%2B1");
    auto sp = u.get_search_params();
    EXPECT_STREQ(sp.at("a")[0].c_str(), "+");
    EXPECT_STREQ(sp.at("b")[0].c_str(), "1+1");
}

UTEST(UrlTest, SearchParamsMixedPlusAndPercent) {
    urlparser::url u("https://example.com/?q=a+b%20c+caf%C3%A9");
    EXPECT_STREQ(u.get_search_params().at("q")[0].c_str(), "a b c caf\xc3\xa9");
}

UTEST(UrlTest, SearchParamsMalformedEscapesAreKept) {
    urlparser::url u("https://example.com/?a=100%&b=%zz&c=%2");
    auto sp = u.get_search_params();
    EXPECT_STREQ(sp.at("a")[0].c_str(), "100%");
    EXPECT_STREQ(sp.at("b")[0].c_str(), "%zz");
    EXPECT_STREQ(sp.at("c")[0].c_str(), "%2");
}

UTEST(UrlTest, SearchParamsSkipsEmptySegments) {
    urlparser::url u("https://example.com/?a=1&&b=2&");
    auto sp = u.get_search_params();
    ASSERT_EQ(sp.size(), (size_t)2);
}

UTEST(UrlTest, PercentCodecDecodeStillLeavesPlusAlone) {
    // The generic RFC 3986 unquote must NOT gain form-decoding's '+' rule.
    EXPECT_STREQ(urlparser::percent_codec::decode("a+b").c_str(), "a+b");
}

// --- build_search_params(): the inverse of get_search_params() ----------
// Expected values verified against ada_url.replace_search_params().

UTEST(UrlTest, BuildSearchParamsSimplePairs) {
    std::vector<std::pair<std::string, std::string>> pairs = {{"a", "1"}, {"b", "2"}};
    EXPECT_STREQ(urlparser::build_search_params(pairs).c_str(), "a=1&b=2");
}

UTEST(UrlTest, BuildSearchParamsSpaceBecomesPlus) {
    std::vector<std::pair<std::string, std::string>> pairs = {{"q", "hello world"}};
    EXPECT_STREQ(urlparser::build_search_params(pairs).c_str(), "q=hello+world");
}

UTEST(UrlTest, BuildSearchParamsEscapesReservedChars) {
    std::vector<std::pair<std::string, std::string>> pairs = {{"a", "b&c=d"}};
    EXPECT_STREQ(urlparser::build_search_params(pairs).c_str(), "a=b%26c%3Dd");
}

UTEST(UrlTest, BuildSearchParamsEncodesUnicodeAsUtf8Percent) {
    std::vector<std::pair<std::string, std::string>> pairs = {{"x", "caf\xc3\xa9"}};
    EXPECT_STREQ(urlparser::build_search_params(pairs).c_str(), "x=caf%C3%A9");
}

UTEST(UrlTest, BuildSearchParamsTildeIsEscaped) {
    // Narrower safe set than percent_codec's RFC 3986 unreserved: '~' is
    // NOT safe in a query string, unlike a path.
    std::vector<std::pair<std::string, std::string>> pairs = {{"k", "~"}};
    EXPECT_STREQ(urlparser::build_search_params(pairs).c_str(), "k=%7E");
}

UTEST(UrlTest, BuildSearchParamsKeepsStarDotDashUnderscore) {
    std::vector<std::pair<std::string, std::string>> pairs = {{"k", "*-._"}};
    EXPECT_STREQ(urlparser::build_search_params(pairs).c_str(), "k=*-._");
}

UTEST(UrlTest, BuildSearchParamsRepeatedKeysInOrderNoDedup) {
    std::vector<std::pair<std::string, std::string>> pairs = {{"a", "1"}, {"a", "2"}};
    EXPECT_STREQ(urlparser::build_search_params(pairs).c_str(), "a=1&a=2");
}

UTEST(UrlTest, BuildSearchParamsEmptyValueKeepsEquals) {
    std::vector<std::pair<std::string, std::string>> pairs = {{"a", ""}};
    EXPECT_STREQ(urlparser::build_search_params(pairs).c_str(), "a=");
}

UTEST(UrlTest, BuildSearchParamsEmptyPairsGivesEmptyString) {
    std::vector<std::pair<std::string, std::string>> pairs;
    EXPECT_STREQ(urlparser::build_search_params(pairs).c_str(), "");
}

UTEST(UrlTest, BuildSearchParamsRoundTripsWithGetSearchParams) {
    urlparser::url u("https://example.com/?q=hello+world&a=1&a=2");
    auto sp = u.get_search_params();
    // Round-trip through the map overload; key order isn't guaranteed, so
    // parse the rebuilt string back and compare the decoded maps instead
    // of the raw strings.
    std::string rebuilt = urlparser::build_search_params(sp);
    urlparser::url u2("https://example.com/?" + rebuilt);
    EXPECT_TRUE(u2.get_search_params() == sp);
}

UTEST(UrlTest, FreeFunctionNormalizeMatchesMethod) {
    EXPECT_STREQ(urlparser::normalize("https://example.com:443/a/../b").c_str(),
                 "https://example.com/b");
}

// --- whitespace trimming (parser-level fix, not normalized()-specific) ---

UTEST(UrlTest, ConstructorTrimsLeadingAndTrailingWhitespace) {
    urlparser::url u("  https://example.com/path  ");
    EXPECT_STREQ(u.str().c_str(), "https://example.com/path");
}

UTEST(UrlTest, ConstructorTrimsTabsAndNewlines) {
    urlparser::url u("\t\nhttps://example.com/path\n\t");
    EXPECT_STREQ(u.str().c_str(), "https://example.com/path");
}
