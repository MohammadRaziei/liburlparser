//
// Created by mohammad on 5/20/23.
//
#include <iostream>
#include <variant>
#include "common.h"
#include "urlparser.h"


#define show_attr(url, attr) \
    std::cout << std::boolalpha << #attr << " : " << url.attr() << std::endl
#define show(var) \
    std::cout << std::boolalpha << #var << " : " << (var) << std::endl

// HTTPS (Hypertext Transfer Protocol Secure): A secure version of HTTP that
// encrypts data transmission to prevent unauthorized access. URLs that begin
// with "https://" are used for accessing secure websites. FTP: (File Transfer
// Protocol): Used for transferring files between computers on a network. URLs
// that begin with "ftp://" are used for accessing files on an FTP server. SSH:
// (Secure Shell): Used for secure remote access to a computer or server. URLs
// that begin with "ssh://" are used for accessing a remote computer or server
// via SSH. SMTP: (Simple Mail Transfer Protocol): Used for sending email
// messages between servers. URLs that begin with "smtp://" are used for sending
// email messages. IMAP: (Internet Message Access Protocol): Used for retrieving
// email messages from a server. URLs that begin with "imap://" are used for
// accessing email messages. POP: (Post Office Protocol): Used for retrieving
// email messages from a server. URLs that begin with "pop://" are used for
// accessing email messages.

int main() {
    // --- Domain-name hosts: urlparser::hostname, reached via url::host() ---
    {
        const urlparser::url url(
            "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
            "home?o=10&k=helloworld#aboutus",
            true);

        // url.host() is a urlparser::host (hostname/ipv4/ipv6, whichever
        // this URL's host actually is); this URL's host is a domain name.
        const auto* h = url.host().try_hostname();
        show(h != nullptr);
        show(h->suffix());
        show(h->domain());
        show(h->subdomain());
        show(h->domain_name());
    }

    // --- IPv4 host: urlparser::ipv4, with its integer/arithmetic API ---
    {
        const urlparser::url url("http://192.168.1.1:8080/admin", true);
        const auto* v4 = url.host().try_ipv4();
        show(v4 != nullptr);
        show(v4->str());
        show(v4->to_uint32());

        auto next = *v4 + 1;
        show(next.str());
        auto broadcast = urlparser::ipv4::from_uint32(0xFFFFFFFFu);
        show(broadcast.str());
        show((broadcast + 1).str());  // wraps around to 0.0.0.0
    }

    // --- IPv6 host: urlparser::ipv6 ---
    {
        const urlparser::url url("http://[2001:db8::1]:8080/x", true);
        const auto* v6 = url.host().try_ipv6();
        show(v6 != nullptr);
        show(v6->str());
        show((*v6 + 1).str());
        show(v6->high64());
        show(v6->low64());
    }

    // --- std::visit for generic handling of whichever host type it is ---
    // (.variant() is the escape hatch for callers who want std::visit/
    // std::get instead of the named is_*/get_*/try_* methods below.)
    {
        for (const char* raw : {"https://example.com/", "http://8.8.8.8/", "http://[::1]/"}) {
            const urlparser::url url(raw, false);
            std::visit(
                [](const auto& host) { std::cout << "  -> " << host.str() << "\n"; },
                url.host().variant());
        }
    }

    // --- host(...)/host::from_url(): classify a bare host or a full URL
    // directly, without going through urlparser::url at all ---
    {
        // Classifying an already-isolated host string:
        show(urlparser::host("example.com").is_hostname());
        show(urlparser::host("192.0.2.1").is_ipv4());
        show(urlparser::host("[::1]").is_ipv6());

        // Extracting + classifying straight from a full URL in one step -
        // .str() gets the text form regardless of which alternative it
        // turned out to be, no try_*/get_*/visit needed.
        urlparser::host h = urlparser::host::from_url("http://192.168.1.1:8080/x");
        show(h.str());
    }

    const urlparser::url url(
        "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
        "home?o=10&k=helloworld#aboutus",
        true);

    urlparser::url _url = url;
    show(_url);
    {
        urlparser::url __url(
            "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
            "home?o=10&k=helloworld#aboutus",
            true);
        __url = _url;
        show(_url == __url);
    }
    show(_url == url);

    urlparser::host _host = urlparser::host::wrap(urlparser::hostname::from_url(_url.str()));
    _host = _url.host();
    show(_host);

    tic;
    for (int i = 0; i < 10'000'000; ++i)
        urlparser::hostname host("www.ee.aut.ac.ir");
    toc;

    tic;
    for (int i = 0; i < 10'000'000; ++i)
        urlparser::hostname::from_url(
            "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
            "home?o=10&k=helloworld#aboutus",
            true);
    toc;

    // --- speed of from_url() across every host kind: hostname/ipv4/ipv6,
    // and host (which additionally has to classify which of the three it
    // is before parsing) ---
    tic;
    for (int i = 0; i < 10'000'000; ++i)
        urlparser::ipv4::from_url("http://192.168.1.1:8080/admin");
    toc;

    tic;
    for (int i = 0; i < 10'000'000; ++i)
        urlparser::ipv6::from_url("http://[2001:db8::1]:8080/x");
    toc;

    tic;
    for (int i = 0; i < 10'000'000; ++i)
        urlparser::host::from_url(
            "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
            "home?o=10&k=helloworld#aboutus",
            true);
    toc;

    tic;
    for (int i = 0; i < 10'000'000; ++i)
        urlparser::host::from_url("http://192.168.1.1:8080/admin");
    toc;

    show(urlparser::hostname("www.ee.aut.ac.ir").suffix());
    show(urlparser::hostname("aut.ac.ir").full_domain());
    show(urlparser::hostname("ee.aut.ac.ir").full_domain());

    show_attr(url, is_psl_loaded);
    show(url);
    show_attr(url, str);
    show_attr(url, port);
    show_attr(url, protocol);
    show_attr(url, host);
    show_attr(url, host_text);
    show_attr(url, query);
    show_attr(url, fragment);
    show_attr(url, userinfo);
    show_attr(url, params);
    show_attr(url, abspath);
    //
    show(urlparser::hostname("ee.aut.ac.ir"));
    show(urlparser::url("https://ee.aut.ac.ir/about"));

    show(urlparser::hostname::from_url("http://mohammad:123@www.google.com?about", true));
    show(urlparser::hostname::from_url("mohammad:123@www.google.com?about", true));
    show(urlparser::hostname::from_url("www.google.com?about", true));
    show(urlparser::hostname::from_url("www.google.com/?about", true));
    show(urlparser::hostname::from_url("www.google.com", true));

    show(urlparser::url("http://mohammad:123@www.google.com?about", true).host());
    show(urlparser::url("https://www.p30download.ir", false).host());

    show(urlparser::url("http://mohammad:123@www.google.com?about", true).host_text());
    // note: hostname(str) treats str literally as a host - use hostname::from_url() to parse a full URL
    show(urlparser::hostname("http://mohammad:123@www.google.com?about", true) == "google.com");
    const urlparser::host host2 = urlparser::host::wrap(urlparser::hostname::from_url("http://mohammad:123@www.google.com?about", true));
    const urlparser::url url2("http://mohammad:123@www.google.com?about", true);
    tic;
    const auto a = url2.host_text();
    toc;
    show(a);

    printf("\ngood bye :)\n");
    return 0;
}
