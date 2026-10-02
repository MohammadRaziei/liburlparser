#include <iostream>

#include "urlparser.h"

int main() {
    urlparser::url u("https://www.example.co.uk:8080/path?q=1");
    std::cout << "protocol: " << u.protocol() << "\n";
    if (const auto* h = u.host().try_hostname()) {
        std::cout << "domain:   " << h->domain() << "\n";
        std::cout << "suffix:   " << h->suffix() << "\n";
    }
    return 0;
}
