//
// Created by mohammad on 5/20/23.
//
#include <iostream>
#include "common.h"
#include "urlparser.h"

 



#undef URL_INITIALIZE_PSL

#define show_attr(url, attr) std::cout << std::boolalpha << #attr << " : " << url.attr() << std::endl
#define show(var) std::cout << std::boolalpha << #var << " : " << var << std::endl

//HTTPS (Hypertext Transfer Protocol Secure): A secure version of HTTP that encrypts data transmission to prevent unauthorized access. URLs that begin with "https://" are used for accessing secure websites.
//FTP: (File Transfer Protocol): Used for transferring files between computers on a network. URLs that begin with "ftp://" are used for accessing files on an FTP server.
//SSH: (Secure Shell): Used for secure remote access to a computer or server. URLs that begin with "ssh://" are used for accessing a remote computer or server via SSH.
//SMTP: (Simple Mail Transfer Protocol): Used for sending email messages between servers. URLs that begin with "smtp://" are used for sending email messages.
//IMAP: (Internet Message Access Protocol): Used for retrieving email messages from a server. URLs that begin with "imap://" are used for accessing email messages.
//POP: (Post Office Protocol): Used for retrieving email messages from a server. URLs that begin with "pop://" are used for accessing email messages.


int main(int argc, char* argv[]){
//    TLD::Url::loadPslFromString("");
//    TLD::Url url("hi");
    tic;
    TLD::Url url("https://m.raziei:1234@www.ee.aut.ac.ir:80/home?o=10&k=helloworld#aboutus");
    toc;

    tic;
    for(int i = 0; i < 1000; ++i)
        TLD::Url::Host host("www.ee.aut.ac.ir");
    toc;

    show(TLD::Url::Host("https://www.ee.aut.ac.ir").suffix());
//    TLD::Url url("a.ir3");
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
    return 0;
}
