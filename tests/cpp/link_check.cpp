// Links against the real liburlparser::urlparser target (static or shared) and calls
// at least one symbol of every exported family. Built with URLPARSER_SHARED=ON the
// library has hidden default visibility, so a public symbol that lacks URLPARSER_API
// makes this program fail to link instead of passing silently.
#include <urlparser.h>

#include <cstdio>
#include <sstream>
#include <string>

namespace {
int failures = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        ++failures;
    }
}
}  // namespace

int main() {
    const urlparser::url u("https://user:p%40ss@example.com:8443/a/./b/../c?x=1&y=2#frag");
    check(u.protocol() == "https", "url::protocol");
    check(u.username() == "user", "url::username");
    check(u.password() == "p%40ss", "url::password stays raw");
    check(u.port() == 8443 && u.port_or_default() == 8443, "url::port / port_or_default");
    check(u.abspath() == "/a/c", "url::abspath");
    check(u.normalized() == "https://user:p%40ss@example.com:8443/a/c?x=1&y=2#frag", "url::normalized");
    check(!u.get_search_params().empty(), "url::get_search_params");
    check(std::get_if<urlparser::hostname>(&u.host().variant()) != nullptr, "url::host classifies a hostname");

    check(urlparser::url::default_port("https") == 443, "url::default_port");
    check(urlparser::url("http://example.com/").port_or_default() == 80, "port_or_default falls back to the scheme");

    const urlparser::url v6("http://[::1]:8080/");
    check(v6.host_text() == "[::1]" && v6.port() == 8080, "IPv6 literal host");
    check(std::get_if<urlparser::ipv6>(&v6.host().variant()) != nullptr, "url::host classifies an ipv6");

    check(urlparser::ipv4("192.0.2.1").to_uint32() == 0xC0000201u, "ipv4");
    check(urlparser::ipv6("::1").str() == "::1", "ipv6");
    check(urlparser::host("192.0.2.1").str() == "192.0.2.1", "host");

    urlparser::hostname h("www.example.co.uk");
    check(!urlparser::url::is_psl_loaded() || h.suffix() == "co.uk", "hostname::suffix / psl");
    check(urlparser::psl::instance().is_loaded() == urlparser::url::is_psl_loaded(), "psl::instance");

    check(urlparser::percent_codec::decode("a%20b") == "a b", "percent_codec::decode");
    check(urlparser::percent_codec::encode("a b") == "a%20b", "percent_codec::encode");
    check(urlparser::idna::to_ascii("caf\xC3\xA9.com") == "xn--caf-dma.com", "idna::to_ascii");
    check(urlparser::resolve("https://example.com/a/b", "../c") == "https://example.com/c", "resolve");
    check(urlparser::build_search_params({{"q", "a b"}}) == "q=a+b", "build_search_params");
    check(urlparser::scp_url::is_scp_like("git@github.com:user/repo.git"), "scp_url::is_scp_like");
    check(urlparser::scp_url("git@github.com:user/repo.git").as_url().protocol() == "ssh", "scp_url");

    std::ostringstream os;
    os << u << ' ' << h << ' ' << urlparser::ipv4("192.0.2.1");
    check(!os.str().empty(), "operator<<");

    if (failures == 0) std::puts("link_check: ok");
    return failures == 0 ? 0 : 1;
}
