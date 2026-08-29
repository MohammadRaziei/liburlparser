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
        const urlparser::url url(
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
        const urlparser::url url(
            "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
            "home?o=10&k=helloworld#aboutus",
            true);
        toc;


        tic;
        const auto hostName = url.full_domain();
        toc;
        tic;
        const auto host = url.host();
        toc;

        show(hostName);
        show(host);
    }

    const urlparser::url url(
        "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
        "home?o=10&k=helloworld#aboutus",
        true);

    urlparser::url _url = url;
    show(_url);
    show(_url.suffix());
    {
        urlparser::url __url(
            "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
            "home?o=10&k=helloworld#aboutus",
            true);
        __url = _url;
        show(__url.suffix());
        show(_url == __url);
    }
    show(_url.suffix());
    show(_url == url);

    urlparser::host _host = urlparser::host::from_url(_url.str());
    _host = _url.host();

    show(_host);

    tic;
    for (int i = 0; i < 10'000'000; ++i)
        urlparser::host host("www.ee.aut.ac.ir");
    toc;

    tic;
    for (int i = 0; i < 10'000'000; ++i)
        urlparser::host::from_url(
            "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
            "home?o=10&k=helloworld#aboutus",
            true);
    toc;
    urlparser::host host = urlparser::host::from_url(
        "https://m.raziei:1234@www.ee.aut.ac.ir:80/"
        "home?o=10&k=helloworld#aboutus");

    show(urlparser::host("www.ee.aut.ac.ir").suffix());
    show(urlparser::host("aut.ac.ir").full_domain());
    show(urlparser::host("ee.aut.ac.ir").full_domain());

    show_attr(url, is_psl_loaded);
    show(url);
    show_attr(url, str);
    show_attr(url, port);
    show_attr(url, domain);
    show_attr(url, subdomain);
    show_attr(url, protocol);
    show_attr(url, suffix);
    show_attr(url, host);
    show_attr(url, full_domain);
    show_attr(url, domain_name);
    show_attr(url, query);
    show_attr(url, port);
    show_attr(url, fragment);
    show_attr(url, userinfo);
    show_attr(url, params);
    show_attr(url, abspath);
    //
    show(urlparser::host("ee.aut.ac.ir"));
    show(urlparser::url("https://ee.aut.ac.ir/about"));
    show(urlparser::url("http://www.google.com").subdomain());
    show(urlparser::url("http://www.google.com", true).subdomain());
    show(urlparser::url("http://www.google.com").domain());
    show(urlparser::url("http://www.google.com", true).subdomain());
    show(urlparser::url("http://www.google.com", true).domain());

    show(urlparser::host::from_url("http://mohammad:123@www.google.com?about", true));
    show(urlparser::host::from_url("mohammad:123@www.google.com?about", true));
    show(urlparser::host::from_url("www.google.com?about", true));
    show(urlparser::host::from_url("www.google.com/?about", true));
    show(urlparser::host::from_url("www.google.com", true));

    show(urlparser::url("http://mohammad:123@www.google.com?about", true).host());
    show(urlparser::url("https://www.p30download.ir", false).host());

    show(urlparser::url("http://mohammad:123@www.google.com?about", true).full_domain());
    show(urlparser::host("http://mohammad:123@www.google.com?about", true) == "google.com"); // note: host(str) treats str literally as a host - use host::from_url() to parse a full URL
    const urlparser::host host2 = urlparser::host::from_url("http://mohammad:123@www.google.com?about", true);
    const urlparser::url url2("http://mohammad:123@www.google.com?about", true);
    tic;
    const auto a = url2.full_domain();
    toc;
    show(a);

    printf("\ngood bye :)\n");
    return 0;
}
