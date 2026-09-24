#include "utest.h"
#include <string>

#include "urlparser.h"

// Gap identified by comparing against ada-url/ada: liburlparser accepts
// non-ASCII (Unicode) hostnames without error, but does not normalize them
// to their canonical ASCII/Punycode form (see the "ASCII-only per RFC 3986"
// comment in include/urlparser.h). Two hosts that are the same domain
// (e.g. "café.com" and "xn--caf-dma.com") therefore currently compare as
// different hostnames. urlparser::idna::to_ascii() closes that gap by
// delegating to ada::idna::to_ascii(), the same WHATWG/UTS-46 compliant
// implementation ada itself uses.

UTEST(idna, ascii_passthrough) {
    ASSERT_STREQ(urlparser::idna::to_ascii("example.com").c_str(), "example.com");
}

UTEST(idna, already_punycoded_is_unchanged) {
    ASSERT_STREQ(urlparser::idna::to_ascii("xn--7eleven-506c.com").c_str(),
                 "xn--7eleven-506c.com");
}

UTEST(idna, simple_unicode_label) {
    ASSERT_STREQ(urlparser::idna::to_ascii("caf\xc3\xa9.com").c_str(),
                 "xn--caf-dma.com");
}

UTEST(idna, german_umlaut) {
    ASSERT_STREQ(urlparser::idna::to_ascii("m\xc3\xbcnchen.de").c_str(),
                 "xn--mnchen-3ya.de");
}

UTEST(idna, mixed_ascii_and_unicode_subdomain) {
    ASSERT_STREQ(
        urlparser::idna::to_ascii("www.7-Eleven.com").c_str(),
        "www.7-eleven.com");  // pure ASCII label: lowercased, not touched otherwise
}

UTEST(idna, mixed_ascii_and_unicode_label) {
    // café-shop -> a genuinely mixed ASCII+non-ASCII label
    ASSERT_STREQ(urlparser::idna::to_ascii("caf\xc3\xa9-shop.com").c_str(),
                 "xn--caf-shop-d1a.com");
}

UTEST(idna, non_latin_script_both_labels) {
    // دامنه.ایران
    ASSERT_STREQ(urlparser::idna::to_ascii(
                     "\xd8\xaf\xd8\xa7\xd9\x85\xd9\x86\xd9\x87."
                     "\xd8\xa7\xdb\x8c\xd8\xb1\xd8\xa7\xd9\x86").c_str(),
                 "xn--mgbp1eef.xn--mgba3a4f16a");
}

UTEST(idna, hostname_from_url_now_normalizes_unicode) {
    // End-to-end: hostname::from_url should expose the same normalization
    // through a new normalized_ascii() accessor, without changing str()
    // (which stays the original, human-readable form for display/back-refs
    // into the source URL).
    urlparser::hostname h = urlparser::hostname::from_url("https://caf\xc3\xa9.com/menu");
    ASSERT_STREQ(h.normalized_ascii().c_str(), "xn--caf-dma.com");
    ASSERT_STREQ(h.str().c_str(), "caf\xc3\xa9.com");
}
