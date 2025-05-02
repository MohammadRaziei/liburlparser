# URL Parser Library

## Introduction

URL Parser is a high-performance C++ library designed for parsing and analyzing URLs. It provides robust functionality to extract various components of a URL such as protocol, domain, subdomain, suffix, path, and query parameters. This library is optimized for speed and efficiency, making it ideal for applications that need to process large numbers of URLs.

## Key Features

- **High Performance**: Optimized C++ implementation for fast URL parsing
- **Comprehensive URL Parsing**: Extract all components of a URL including protocol, domain, subdomain, suffix, path, query parameters, and fragments
- **Public Suffix List Support**: Accurate domain recognition using the public suffix list
- **Clean API Design**: Separate `Url` and `Host` classes for better code organization
- **Cross-Platform Compatibility**: Works on Windows, Linux, and macOS
- **Automatic PSL Updates**: Updates the public suffix list automatically during build

## Core Classes

### TLD::Url Class

The `TLD::Url` class is the main class for parsing and analyzing complete URLs. It provides methods to access all components of a URL.

#### Key Methods and Properties

- `protocol()`: Returns the protocol (e.g., "http", "https")
- `userinfo()`: Returns the user information part of the URL
- `host()`: Returns a `TLD::Host` object representing the host part
- `port()`: Returns the port number
- `abspath()`: Returns the absolute path
- `query()`: Returns the query string
- `params()`: Returns the query parameters as a map
- `fragment()`: Returns the fragment (anchor)
- `str()`: Returns the complete URL as a string

### TLD::Host Class

The `TLD::Host` class is used for parsing and analyzing the host part of a URL.

#### Key Methods and Properties

- `domain()`: Returns the domain part
- `subdomain()`: Returns the subdomain part
- `suffix()`: Returns the suffix (TLD)
- `domainName()`: Returns the domain name (domain + suffix)
- `fulldomain()`: Returns the full domain (subdomain + domain + suffix)
- `str()`: Returns the host as a string
- `static fromUrl(const std::string& url, bool ignore_www = false)`: Creates a Host object from a URL

## Usage Examples

### Basic URL Parsing

```cpp
#include <iostream>
#include "urlparser.h"

int main() {
    // Parse a URL
    TLD::Url url("https://www.example.com/path?param=value#section");
    
    // Access URL components
    std::cout << "Protocol: " << url.protocol() << std::endl;
    std::cout << "Domain: " << url.domain() << std::endl;
    std::cout << "Subdomain: " << url.subdomain() << std::endl;
    std::cout << "Suffix: " << url.suffix() << std::endl;
    std::cout << "Path: " << url.abspath() << std::endl;
    std::cout << "Query: " << url.query() << std::endl;
    std::cout << "Fragment: " << url.fragment() << std::endl;
    
    return 0;
}
```

### Working with Hosts

```cpp
#include <iostream>
#include "urlparser.h"

int main() {
    // Create a Host object directly
    TLD::Host host1("www.example.com");
    
    // Create a Host object from a URL
    TLD::Host host2 = TLD::Host::fromUrl("https://www.example.com/path?param=value");
    
    // Access Host components
    std::cout << "Domain: " << host1.domain() << std::endl;
    std::cout << "Subdomain: " << host1.subdomain() << std::endl;
    std::cout << "Suffix: " << host1.suffix() << std::endl;
    std::cout << "Domain Name: " << host1.domainName() << std::endl;
    
    // Compare hosts
    if (host1 == host2) {
        std::cout << "Hosts are equal" << std::endl;
    }
    
    return 0;
}
```

### Advanced Example

```cpp
#include <iostream>
#include "urlparser.h"

int main() {
    // Parse a complex URL
    const TLD::Url url(
        "https://user:password@www.subdomain.example.co.uk:8080/path/to/resource?param1=value1&param2=value2#section",
        true  // ignore_www parameter
    );
    
    // Access URL components
    std::cout << "Protocol: " << url.protocol() << std::endl;
    std::cout << "User Info: " << url.userinfo() << std::endl;
    std::cout << "Domain: " << url.domain() << std::endl;
    std::cout << "Subdomain: " << url.subdomain() << std::endl;
    std::cout << "Suffix: " << url.suffix() << std::endl;
    std::cout << "Port: " << url.port() << std::endl;
    std::cout << "Path: " << url.abspath() << std::endl;
    std::cout << "Query: " << url.query() << std::endl;
    std::cout << "Fragment: " << url.fragment() << std::endl;
    
    // Access query parameters
    auto params = url.params();
    for (const auto& param : params) {
        std::cout << "Parameter: " << param.first << " = " << param.second << std::endl;
    }
    
    // Get the host object
    TLD::Host host = url.host();
    std::cout << "Full Domain: " << host.fulldomain() << std::endl;
    
    return 0;
}
```

### Performance Benchmarking

```cpp
#include <iostream>
#include <chrono>
#include "urlparser.h"

// Simple timing macro
#define TIME_FUNCTION(func) \
    { \
        auto start = std::chrono::high_resolution_clock::now(); \
        func; \
        auto end = std::chrono::high_resolution_clock::now(); \
        std::chrono::duration<double, std::milli> elapsed = end - start; \
        std::cout << "Time taken: " << elapsed.count() << " ms" << std::endl; \
    }

int main() {
    // Benchmark Host creation
    TIME_FUNCTION({
        for (int i = 0; i < 1'000'000; ++i) {
            TLD::Host host("www.example.com");
        }
    });
    
    // Benchmark Host creation from URL
    TIME_FUNCTION({
        for (int i = 0; i < 1'000'000; ++i) {
            TLD::Host::fromUrl("https://www.example.com/path?param=value");
        }
    });
    
    return 0;
}
```

## Public Suffix List

The library uses the Public Suffix List (PSL) to accurately identify domain suffixes. The PSL is automatically downloaded during the build process, but you can also load it manually:

```cpp
#include "urlparser.h"

int main() {
    // Check if PSL is loaded
    bool pslLoaded = TLD::Host::isPslLoaded();
    
    // Load PSL from a file
    if (!pslLoaded) {
        TLD::Host::loadPslFromPath("/path/to/public_suffix_list.dat");
    }
    
    // Or load PSL from a string
    std::string pslContent = "..."; // PSL content
    TLD::Host::loadPslFromString(pslContent);
    
    return 0;
}
```

## Building and Installation

### Prerequisites

- C++17 compatible compiler
- CMake 3.19 or higher

### Build Steps

```
git clone https://github.com/mohammadraziei/liburlparser
mkdir -p build && cd build
cmake ..
make
sudo make install
```

### Running Tests

```
make test
```

### Generating Documentation

```
make docs
```

## Performance Comparison

URL Parser is designed for high performance. In benchmarks, it consistently outperforms other URL parsing libraries:

| Library | Function | Time (Extract from Host) | Time (Extract from URL) |
|---------|----------|--------------------------|-------------------------|
| liburlparser | Host | 1.12s | 2.10s |
| Other libraries | - | 1.50s - 34.48s | 2.24s - 57.87s |

*Tests were run on a file containing 10 million random domains and 1 million random URLs.*

## License

This library is distributed under the MIT License. See the LICENSE file for more information.
