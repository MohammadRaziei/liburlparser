//
// Created by mohammad on 5/20/23.
//
#include <iostream>
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
    {
        tic;
        const TLD::Url url(
            "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
            "home?o=10&k=helloworld#aboutus",
            true);
        toc;

        tic;
        const std::string suffix = url.suffix();
        toc;
        tic;
        const std::string domain = url.domain();
        toc;

        show(suffix);
        show(domain);
    }
    {
        tic;
        const TLD::Url url(
            "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
            "home?o=10&k=helloworld#aboutus",
            true);
        toc;


        tic;
        const auto hostName = url.fulldomain();
        toc;
        tic;
        const auto host = url.host();
        toc;

        show(hostName);
        show(host);
    }

    const TLD::Url url(
        "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
        "home?o=10&k=helloworld#aboutus",
        true);

    TLD::Url _url = url;
    show(_url);
    show(_url.suffix());
    {
        TLD::Url __url(
            "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
            "home?o=10&k=helloworld#aboutus",
            true);
        __url = _url;
        show(__url.suffix());
        show(_url == __url);
    }
    show(_url.suffix());
    show(_url == url);

    TLD::Host _host = TLD::Host::fromUrl(_url.str());
    _host = _url.host();

    show(_host);

    tic;
    for (int i = 0; i < 1'000'000; ++i)
        TLD::Host host("www.ee.aut.ac.ir");
    toc;

    tic;
    for (int i = 0; i < 1'000'000; ++i)
        TLD::Host::fromUrl(
            "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
            "home?o=10&k=helloworld#aboutus",
            true);
    toc;
    TLD::Host host = TLD::Host::fromUrl(
        "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
        "home?o=10&k=helloworld#aboutus");

    show(TLD::Host("www.ee.aut.ac.ir").suffix());
    show(TLD::Host("aut.ac.ir").fulldomain());
    show(TLD::Host("ee.aut.ac.ir").fulldomain());

    show_attr(url, isPslLoaded);
    show(url);
    show_attr(url, str);
    show_attr(url, port);
    show_attr(url, domain);
    show_attr(url, subdomain);
    show_attr(url, protocol);
    show_attr(url, suffix);
    show_attr(url, host);
    show_attr(url, fulldomain);
    show_attr(url, domainName);
    show_attr(url, query);
    show_attr(url, port);
    show_attr(url, fragment);
    show_attr(url, userinfo);
    show_attr(url, params);
    show_attr(url, abspath);
    //
    show(TLD::Host("ee.aut.ac.ir"));
    show(TLD::Url("https://ee.aut.ac.ir/about"));
    show(TLD::Url("http://www.google.com").subdomain());
    show(TLD::Url("http://www.google.com", true).subdomain());
    show(TLD::Url("http://www.google.com").domain());
    show(TLD::Url("http://www.google.com", true).subdomain());
    show(TLD::Url("http://www.google.com", true).domain());

    show(TLD::Host::fromUrl("http://mohammad:123@www.google.com?about", true));
    show(TLD::Host::fromUrl("mohammad:123@www.google.com?about", true));
    show(TLD::Host::fromUrl("www.google.com?about", true));
    show(TLD::Host::fromUrl("www.google.com/?about", true));
    show(TLD::Host::fromUrl("www.google.com", true));

    show(TLD::Url("http://mohammad:123@www.google.com?about", true).host());
    show(TLD::Url("https://www.p30download.ir", false).host());

    show(TLD::Url("http://mohammad:123@www.google.com?about", true).fulldomain()); // TODO: fix hostName Issue for ignore_www
    show(TLD::Host("http://mohammad:123@www.google.com?about", true) == "google.com"); // TODO: fix hostName Issue for ignore_www
    const TLD::Host host2 = TLD::Host::fromUrl("http://mohammad:123@www.google.com?about", true);
    const TLD::Url url2("http://mohammad:123@www.google.com?about", true);
    tic;
    const auto a = url2.fulldomain();
    toc;
    show(a); // TODO: add clone version to copy without sharing

    printf("\ngood bye :)\n");
    return 0;
}
