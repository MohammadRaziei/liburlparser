// Exercises at least one symbol of every exported family of urlparser.h.
//
// This file is compiled twice (see tests/cpp/CMakeLists.txt):
//   1. into test_liburlparser_cpp, together with src/urlparser.cpp (URLPARSER_STATIC);
//   2. alone into test_liburlparser_linked, which links the real liburlparser::urlparser
//      target exactly like an outside consumer would.
// With -DURLPARSER_SHARED=ON the library has hidden default visibility, so a public
// symbol that lacks URLPARSER_API makes the second executable fail to link instead of
// passing silently.
#include "utest.h"

#include <sstream>
#include <string>
#include <variant>

#include "urlparser.h"

UTEST(link_check, url_accessors) {
    const urlparser::url u("https://user:p%40ss@example.com:8443/a/./b/../c?x=1&y=2#frag");
    ASSERT_TRUE(u.protocol() == "https");
    ASSERT_TRUE(u.username() == "user");
    ASSERT_TRUE(u.password() == "p%40ss");
    ASSERT_EQ(u.port(), 8443);
    ASSERT_EQ(u.port_or_default(), 8443);
    ASSERT_TRUE(u.abspath() == "/a/c");
    ASSERT_TRUE(u.normalized() == "https://user:p%40ss@example.com:8443/a/c?x=1&y=2#frag");
    ASSERT_FALSE(u.get_search_params().empty());
    ASSERT_TRUE(std::get_if<urlparser::hostname>(&u.host().variant()) != nullptr);
}

UTEST(link_check, default_port) {
    ASSERT_EQ(urlparser::url::default_port("https"), 443);
    ASSERT_EQ(urlparser::url("http://example.com/").port_or_default(), 80);
}

UTEST(link_check, ipv6_literal_host) {
    const urlparser::url v6("http://[::1]:8080/");
    ASSERT_TRUE(v6.host_text() == "[::1]");
    ASSERT_EQ(v6.port(), 8080);
    ASSERT_TRUE(std::get_if<urlparser::ipv6>(&v6.host().variant()) != nullptr);
}

UTEST(link_check, ip_and_host_types) {
    ASSERT_TRUE(urlparser::ipv4("192.0.2.1").to_uint32() == 0xC0000201u);
    ASSERT_STREQ(urlparser::ipv6("::1").str().c_str(), "::1");
    ASSERT_STREQ(urlparser::host("192.0.2.1").str().c_str(), "192.0.2.1");
}

UTEST(link_check, hostname_and_psl) {
    urlparser::hostname h("www.example.co.uk");
    // With URLPARSER_PSL=OFF the list is empty and suffix() is only the last label.
    ASSERT_TRUE(!urlparser::url::is_psl_loaded() || h.suffix() == "co.uk");
    ASSERT_EQ(urlparser::psl::instance().is_loaded(), urlparser::url::is_psl_loaded());
}

UTEST(link_check, free_functions) {
    ASSERT_STREQ(urlparser::percent_codec::decode("a%20b").c_str(), "a b");
    ASSERT_STREQ(urlparser::percent_codec::encode("a b").c_str(), "a%20b");
    ASSERT_STREQ(urlparser::idna::to_ascii("caf\xC3\xA9.com").c_str(), "xn--caf-dma.com");
    ASSERT_STREQ(urlparser::resolve("https://example.com/a/b", "../c").c_str(), "https://example.com/c");
    ASSERT_STREQ(urlparser::build_search_params({{"q", "a b"}}).c_str(), "q=a+b");
}

UTEST(link_check, scp_url) {
    ASSERT_TRUE(urlparser::scp_url::is_scp_like("git@github.com:user/repo.git"));
    ASSERT_TRUE(urlparser::scp_url("git@github.com:user/repo.git").as_url().protocol() == "ssh");
}

UTEST(link_check, stream_operators) {
    std::ostringstream os;
    os << urlparser::url("https://example.com/") << ' ' << urlparser::hostname("www.example.com") << ' '
       << urlparser::ipv4("192.0.2.1");
    ASSERT_FALSE(os.str().empty());
}
