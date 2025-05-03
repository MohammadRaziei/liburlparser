# liburlparser Library

## Introduction

liburlparser is a high-performance URL parsing library written in C++ with Python bindings. It provides efficient URL parsing and domain extraction capabilities while maintaining a consistent interface across both C++ and Python. This page introduces the core concepts, architecture, and features of liburlparser.

For installation instructions, see Installation. For usage examples, see Quick Start Guide.

## Purpose and Scope

The primary purpose of liburlparser is to parse URLs and extract their components with a focus on accurate domain extraction. The library is designed to:

- Parse URLs into their component parts (protocol, host, path, query parameters, etc.)
- Extract domain information from hostnames, correctly handling public suffixes
- Provide a consistent API across both C++ and Python
- Offer superior performance compared to other URL parsing solutions

## Key Features

- **High Performance**: Optimized C++ implementation for fast URL parsing
- **Comprehensive URL Parsing**: Extract all components of a URL including protocol, domain, subdomain, suffix, path, query parameters, and fragments
- **Public Suffix List Support**: Accurate domain recognition using the public suffix list
- **Clean API Design**: Separate `Url` and `Host` classes for better code organization
- **Cross-Platform Compatibility**: Works on Windows, Linux, and macOS
- **Automatic PSL Updates**: Updates the public suffix list automatically during build

## URL Parsing Process

When parsing a URL, the library follows this general flow:

![URL Parsing Process](url-parsing-process.svg)

## Core Components

The library is built around two primary classes that work together to provide URL parsing functionality:

### TLD::Url Class

The `TLD::Url` class is responsible for parsing complete URLs and extracting all their components. It provides methods to access:

- Protocol (e.g., "https")
- User information
- Host (which is represented as a Host object)
- Port
- Path
- Query parameters
- Fragment

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

The `TLD::Host` class focuses on parsing and extracting domain information from hostnames. It leverages the Public Suffix List (PSL) to correctly handle domain extraction, even for complex cases like "co.uk". It provides methods to access:

- Subdomain (e.g., "www" in "www.example.com")
- Domain (e.g., "example" in "www.example.com")
- Domain name (the combination of domain and suffix)
- Suffix (e.g., "com" in "www.example.com" or "co.uk" in "example.co.uk")

#### Key Methods and Properties

- `domain()`: Returns the domain part
- `subdomain()`: Returns the subdomain part
- `suffix()`: Returns the suffix (TLD)
- `domainName()`: Returns the domain name (domain + suffix)
- `fulldomain()`: Returns the full domain (subdomain + domain + suffix)
- `str()`: Returns the host as a string
- `static fromUrl(const std::string& url, bool ignore_www = false)`: Creates a Host object from a URL

## System Architecture

liburlparser follows a layered architecture with C++ core components providing the parsing functionality and language bindings making this functionality available in Python.

![Architecture Diagram](interface-arch.svg)

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
