#include "utest.h"
#include <string>

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
