#include "utest.h"
#include <string>

#include "urlparser.h"


using urlparser::percent_codec::decode;
using urlparser::percent_codec::encode;
using urlparser::percent_codec::path_safe_set;

// Gap: liburlparser::url::query()/params()/abspath() return the *raw*,
// still percent-encoded strings (by design, matching WHATWG/ada). But
// unlike Python's urllib.parse (quote/unquote), it exposed no public way to
// decode/encode those substrings yourself. This adds that missing pair.

UTEST(percent_codec, decode_plain_string_unchanged) {
    ASSERT_STREQ(decode("hello").c_str(), "hello");
}

UTEST(percent_codec, decode_space) {
    ASSERT_STREQ(decode("hello%20world").c_str(), "hello world");
}

UTEST(percent_codec, decode_utf8_multibyte) {
    // café -> caf%C3%A9
    ASSERT_STREQ(decode("caf%C3%A9").c_str(), "caf\xc3\xa9");
}

UTEST(percent_codec, decode_lowercase_hex_digits) {
    ASSERT_STREQ(decode("caf%c3%a9").c_str(), "caf\xc3\xa9");
}

UTEST(percent_codec, decode_reserved_char) {
    ASSERT_STREQ(decode("hello%20world%26more").c_str(), "hello world&more");
}

UTEST(percent_codec, decode_leaves_malformed_percent_untouched) {
    // trailing '%' with nothing after it
    ASSERT_STREQ(decode("100%").c_str(), "100%");
    // '%' followed by non-hex
    ASSERT_STREQ(decode("100%zz").c_str(), "100%zz");
    // '%' followed by only one hex digit
    ASSERT_STREQ(decode("100%2").c_str(), "100%2");
}

UTEST(percent_codec, decode_empty_string) {
    ASSERT_STREQ(decode("").c_str(), "");
}

UTEST(percent_codec, encode_unreserved_chars_unchanged) {
    ASSERT_STREQ(encode("abcXYZ019-_.~").c_str(), "abcXYZ019-_.~");
}

UTEST(percent_codec, encode_space) {
    ASSERT_STREQ(encode("hello world").c_str(), "hello%20world");
}

UTEST(percent_codec, encode_utf8_multibyte) {
    ASSERT_STREQ(encode("caf\xc3\xa9").c_str(), "caf%C3%A9");
}

UTEST(percent_codec, encode_reserved_chars_by_default) {
    // '/' and '&' are not in the unreserved set, so the default encode()
    // (no explicit safe-set) escapes them.
    ASSERT_STREQ(encode("a/b&c").c_str(), "a%2Fb%26c");
}

UTEST(percent_codec, encode_with_path_safe_set_keeps_slash) {
    ASSERT_STREQ(encode("a/b c", path_safe_set()).c_str(), "a/b%20c");
}

UTEST(percent_codec, encode_empty_string) {
    ASSERT_STREQ(encode("").c_str(), "");
}

UTEST(percent_codec, roundtrip_decode_encode) {
    // encode(decode(x)) doesn't have to equal x byte-for-byte (hex case /
    // choice of safe set can differ), but decode(encode(x)) must always
    // recover x exactly.
    const std::string original = "caf\xc3\xa9 & friends / co.";
    ASSERT_STREQ(decode(encode(original)).c_str(), original.c_str());
}
