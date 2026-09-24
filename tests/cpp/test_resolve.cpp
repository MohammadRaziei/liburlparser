#include "utest.h"
#include <string>

#include "urlparser.h"


using urlparser::resolve;

// Gap: liburlparser had no URL reference-resolution ("join") at all - only
// url::abspath(), which resolves '.'/'..' *within* one already-parsed URL,
// not against a second, separate reference. This implements RFC 3986 §5,
// the algorithm behind JS's `new URL(ref, base)` / ada_url.join_url() /
// Python's urllib.parse.urljoin(). Expected values below were verified
// against ada_url.join_url(base, ref) (same underlying WHATWG algorithm).

UTEST(resolve, relative_path_up_one_level) {
    ASSERT_STREQ(resolve("https://example.com/a/b/c", "../d").c_str(),
                 "https://example.com/a/d");
}

UTEST(resolve, absolute_path_replaces_whole_path) {
    ASSERT_STREQ(resolve("https://example.com/a/b/c", "/absolute").c_str(),
                 "https://example.com/absolute");
}

UTEST(resolve, relative_reference_with_query_replaces_last_segment) {
    ASSERT_STREQ(resolve("https://example.com/a/b/c?x=1", "e?y=2").c_str(),
                 "https://example.com/a/b/e?y=2");
}

UTEST(resolve, dot_slash_relative_from_directory_like_base) {
    ASSERT_STREQ(resolve("https://example.com/a/b/", "./e/f").c_str(),
                 "https://example.com/a/b/e/f");
}

UTEST(resolve, absolute_ref_overrides_base_entirely) {
    ASSERT_STREQ(resolve("https://example.com/a/b/c", "https://other.com/z").c_str(),
                 "https://other.com/z");
}

UTEST(resolve, protocol_relative_ref_keeps_base_scheme) {
    ASSERT_STREQ(resolve("https://example.com/a/b/c", "//other.com/z").c_str(),
                 "https://other.com/z");
}

UTEST(resolve, fragment_only_ref_keeps_everything_else) {
    ASSERT_STREQ(resolve("https://example.com/a/b/c#frag", "#newfrag").c_str(),
                 "https://example.com/a/b/c#newfrag");
}

UTEST(resolve, dot_dot_alone_goes_up_one_level) {
    ASSERT_STREQ(resolve("https://example.com/a/b/c", "..").c_str(),
                 "https://example.com/a/");
}

UTEST(resolve, multiple_dot_dot_cannot_go_above_root) {
    ASSERT_STREQ(resolve("https://example.com/a/b/c", "../../..").c_str(),
                 "https://example.com/");
}

UTEST(resolve, empty_ref_returns_base_unchanged) {
    ASSERT_STREQ(resolve("https://example.com/a/b/c", "").c_str(),
                 "https://example.com/a/b/c");
}

UTEST(resolve, query_only_ref_keeps_base_path) {
    ASSERT_STREQ(resolve("https://example.com/a/b/c", "?q=1").c_str(),
                 "https://example.com/a/b/c?q=1");
}

UTEST(resolve, ref_with_port_in_base_preserved) {
    ASSERT_STREQ(resolve("https://example.com:8080/a/b", "../c").c_str(),
                 "https://example.com:8080/c");
}
