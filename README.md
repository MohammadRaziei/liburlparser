<p align="center">
  <a href="https://github.com/mohammadraziei/liburlparser">
    <img src="https://github.com/MohammadRaziei/liburlparser/raw/master/docs/images/logo/liburlparser-logo-1.svg" alt="Logo">
  </a>
  <h3 align="center">
    Fastest domain extractor library written in C++ with python binding.
  </h3>
  <h4 align="center">
    First and complete library for parsing url in C++ and Python and Command Line
  </h4>
</p>

[![mohammadraziei - liburlparser](https://img.shields.io/static/v1?label=mohammadraziei&message=liburlparser&color=white&logo=github)](https://github.com/mohammadraziei/liburlparser "Go to GitHub repo")
[![stars - liburlparser](https://img.shields.io/github/stars/mohammadraziei/liburlparser?style=social)](https://github.com/mohammadraziei/liburlparser)
[![forks - liburlparser](https://img.shields.io/github/forks/mohammadraziei/liburlparser?style=social)](https://github.com/mohammadraziei/liburlparser)

[![PyPi](https://img.shields.io/pypi/v/liburlparser.svg)](https://pypi.org/project/liburlparser/)
![Python](https://img.shields.io/badge/Python-3.10%20%7C%203.11%20%7C%203.12-blue)
![Cpp](https://img.shields.io/badge/C++-17-blue)

[![GitHub release](https://img.shields.io/github/release/mohammadraziei/liburlparser?include_prereleases=&sort=semver&color=purple)](https://github.com/mohammadraziei/liburlparser/releases/)
[![License](https://img.shields.io/badge/License-MIT-purple)](#license)
[![issues - liburlparser](https://img.shields.io/github/issues/mohammadraziei/liburlparser)](https://github.com/mohammadraziei/liburlparser/issues)

[![SonarCloud](https://sonarcloud.io/images/project_badges/sonarcloud-white.svg)](https://sonarcloud.io/summary/new_code?id=MohammadRaziei_liburlparser)

[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=MohammadRaziei_liburlparser&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=MohammadRaziei_liburlparser)
[![CodeFactor](https://www.codefactor.io/repository/github/mohammadraziei/liburlparser/badge/master)](https://www.codefactor.io/repository/github/mohammadraziei/liburlparser/overview/master)
[![snyk.io](https://snyk.io/advisor/python/liburlparser/badge.svg)](https://snyk.io/advisor/python/liburlparser)

[//]: # "[![View site - GH Pages](https://img.shields.io/badge/View_site-GH_Pages-2ea44f?style=for-the-badge)](https://mohammadraziei.github.io/liburlparser/)"

<!--
![Build](https://github.com/Intsights/PyDomainExtractor/workflows/Build/badge.svg)
[![PyPi](https://img.shields.io/pypi/v/PyDomainExtractor.svg)](https://pypi.org/project/PyDomainExtractor/)
-->
<!--
## Table of Contents

- [Table of Contents](#table-of-contents)
- [About The Project](#about-the-project)
  - [Built With](#built-with)[README.md](README.md)
  - [Performance](#performance)
    - [Extract From Domain](#extract-from-domain)
    - [Extract From URL](#extract-from-url)
  - [Installation](#installation)
- [Usage](#usage)
  - [Extraction](#extraction)
  - [URL Extraction](#url-extraction)
  - [Validation](#validation)
  - [TLDs List](#tlds-list)
- [License](#license)
- [Contact](#contact)
-->

## About The Project

**liburlparser** is a powerful domain extractor library written in C++ with Python bindings. It provides efficient URL parsing capabilities for both C++ and Python, making it a valuable tool for projects that involve working with web addresses.

### Features

Here are some key features of **liburlparser**:

1. **Multiple Language Support**:
   - liburlparser can be used in multiple programming languages, including `Python`, `C++`, and `Shell`.
   - It offers an intuitive interface that remains consistent across both C++ and Python.

2. **Clean Code Design**:
   - The library provides separate classes for each concept: `Url`, and
     `Hostname`/`IPv4`/`IPv6` for whatever a URL's host actually is (RFC
     3986 defines a host as either a domain name or an IP address, never
     both - liburlparser mirrors that directly instead of forcing IP
     addresses through domain-name-shaped fields).
   - This separation allows for cleaner and more organized code when dealing with URLs.

3. **Public Suffix List Support**:
   - liburlparser supports known combinatorial suffixes (e.g., "ac.ir") using the public_suffix_list.
   - It can also handle unknown suffixes (e.g., "comm" in "google.comm").

4. **Automatic Public Suffix List Updates**:
   - Before each build and deployment, liburlparser updates the public_suffix_list automatically.

5. **Hostname Properties**:
   - The `Hostname` class includes properties such as subdomain, domain, domain name, and suffix.

6. **IPv4 / IPv6 Support**:
   - `IPv4`/`IPv6` addresses used as a URL's host are recognized and parsed
     as such - not run through domain-name logic. Both support integer
     conversion and arithmetic (`+`, `-`, `+=`, `-=`, increment/decrement).

7. **URL Properties**:
   - The `Url` class provides properties like protocol, userinfo, host (classified as `Hostname`/`IPv4`/`IPv6`), port, path, query parameters, and fragment.

<!--
* Multiple programming language supported such as `Python`, `C++` and `Shell`
* Intuitive interface and identical in C++ and Python
* Provide separate classes (Url, Hostname, IPv4, IPv6) for the purpose of clean code
* Also support [public_suffix_list](https://publicsuffix.org/list/public_suffix_list.dat) for known combinatorial suffix such as "ac.ir"
* Support unknown suffix like "google.comm" (it detect "comm" as suffix)
* Update public_suffix_list automatically before each build and deploy
* Hostname properties:
  * subdomain
  * domain
  * domain_name
  * suffix
* Url properties:
  * protocol
  * userinfo
  * host (and all the host properties)
  * port
  * path
  * query
  * params
  * fragment
-->

## Usage

### Command Line

```sh
python -m liburlparser --help # show help section
python -m liburlparser --version # show version
python -m liburlparser --url "https://mail.google.com/about" | jq #return as json
python -m liburlparser --host "mail.google.com" | jq # return as json
```

### Python

you can use liburlparser so intutively

all of classes has help section

```python
import liburlparser
help(liburlparser)
print(liburlparser.__version__)

from liburlparser import Url, Hostname, IPv4, IPv6
help(Url)
help(Hostname)
```

parse url and host

```python
from liburlparser import Url, Hostname, Host
## parse url:
url = Url("https://ee.aut.ac.ir/#id") # parse all part of url
print(url, url.host_text, url.fragment, url.host, url.to_dict(), url.to_json())
## url.host is a Hostname, IPv4, or IPv6 object - whichever this URL's host
## actually is (an IP address has no "domain"/"suffix", so it's simply not
## forced through Hostname at all)
host = url.host  # a Hostname, here: ee.aut.ac.ir
# or
host = Hostname("ee.aut.ac.ir")
# or
host = Hostname.from_url("https://ee.aut.ac.ir/#id") # the fastest way for parsing host from url
print(host, host.domain, host.suffix, host.to_dict(), host.to_json())

## for a host you don't know in advance is a domain or an IP, use Host():
host2 = Host("192.168.1.1")   # is_ipv4() -> True
host3 = Host("ee.aut.ac.ir")  # is_hostname() -> True
print(host2.is_ipv4(), host2, int(host2.get_ipv4()))          # True 192.168.1.1 3232235777
print(host3.is_hostname(), host3, host3.get_hostname().domain) # True ee.aut.ac.ir aut
```

IPv4/IPv6 addresses support integer conversion and arithmetic:

```python
from liburlparser import IPv4, IPv6

ip = IPv4("192.168.1.1")
print(int(ip))          # 3232235777
print(ip + 1)            # 192.168.1.2
print(ip - IPv4("192.168.1.0"))  # 1 (distance between two addresses)

ip6 = IPv6("2001:db8::1")
print(ip6.high64, ip6.low64)
print(ip6 + 1)            # 2001:db8::2
```

Also there is some helping api to get better performance for some small tasks

```python
# if you need to extract the host of url as a string without any parsing
host_str = Url.extract_host("https://ee.aut.ac.ir/about") # very fast
```

### C++

there is some examples in [examples](https://github.com/MohammadRaziei/liburlparser/tree/master/examples) folder

```c++
#include "urlparser.h"
...
/// for parsing url
urlparser::url url("https://ee.aut.ac.ir/about");
std::string_view host_text = url.host_text(); // the raw host text, unclassified

/// url.host() is a urlparser::host (hostname/ipv4/ipv6, whichever this
/// URL's host actually is)
if (auto* h = url.host().try_hostname()) {
    std::cout << h->domain() << "." << h->suffix();
}

/// for parsing a host you already have (or a full URL) directly:
urlparser::hostname host("ee.aut.ac.ir");
// or
urlparser::hostname host2 = urlparser::hostname::from_url("https://ee.aut.ac.ir/about");
// or, when you don't know in advance whether it's a domain or an IP:
urlparser::host classified = urlparser::host::from_url("http://192.168.1.1/about"); // -> is_ipv4() true
```

you can see all methods in python we can use in c++ very easily

## Installation

### C++

Requires a C++17 compiler and CMake >= 3.19. The library is a static library plus one header (`urlparser.h`),
and its CMake target is **`urlparser::urlparser`** - the same name no matter how you get it:

#### 1. FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
  liburlparser
  GIT_REPOSITORY https://github.com/mohammadraziei/liburlparser.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(liburlparser)

target_link_libraries(my_app PRIVATE urlparser::urlparser)
```

#### 2. From the pip package

The Python wheel also ships the compiled C++ library, its header and a CMake
package config (same idea as `python -m nanobind --cmake_dir`), so a C++
project can use a plain `pip install liburlparser` without building anything:

```cmake
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(
    COMMAND ${Python3_EXECUTABLE} -m liburlparser --cmake-dir
    OUTPUT_VARIABLE liburlparser_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)

find_package(liburlparser CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE urlparser::urlparser)
```

For build systems that prefer plain paths over the target, `find_package` also sets:

| Variable                 | Value                                                              |
| ------------------------ | ------------------------------------------------------------------ |
| `urlparser_LIB_PATH`     | full path of the compiled library (`liburlparser.a`, `urlparser.lib`, ...) |
| `urlparser_INCLUDE_PATH` | directory containing `urlparser.h`                                 |

The same locations are available from the command line and from Python:

```sh
python -m liburlparser --cmake-dir     # directory with liburlparserConfig.cmake
python -m liburlparser --include-dir   # directory with urlparser.h
python -m liburlparser --lib-dir       # directory with the compiled library
```

```python
import liburlparser
liburlparser.get_cmake_dir(), liburlparser.get_include_dir(), liburlparser.get_lib_dir()
```

A complete example project is in
[examples/find_liburlparser_via_python](examples/find_liburlparser_via_python).

#### 3. Build and install from source

```sh
git clone https://github.com/mohammadraziei/liburlparser
cd liburlparser

# configure and build (-B creates the 'build' directory)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# run the example (on Windows with multi-config generators: ./build/Release/example.exe)
./build/example

# install the library, header and CMake package (add --prefix <dir> to choose where)
cmake --install build
```

Then, in your project (add `-DCMAKE_PREFIX_PATH=<prefix>` if you used a custom prefix):

```cmake
find_package(liburlparser CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE urlparser::urlparser)
```

### Python and Command Line:

Be aware that it requires `python>=3.10`

#### Installation

###### from source

```bash
git clone https://github.com/mohammadraziei/liburlparser
cd liburlparser

pip install .

# Optional: If you want to see build logs
pip install . -v
```

###### pip by [pypi](https://pypi.org/project/liburlparser/)

```sh
pip install liburlparser
```

Or

###### pip by [git](https://github.com/mohammadraziei/liburlparser)

```sh
pip install git+https://github.com/mohammadraziei/liburlparser
```

Or

###### manually

```sh
git clone https://github.com/mohammadraziei/liburlparser
pip install ./liburlparser
```

### Performance

Up-to-date, reproducible results for both C++ and Python (compared against other libraries)
are on the **[Benchmarks page](https://mohammadraziei.github.io/liburlparser/benchmarks/)**.
The report is generated locally and published with the documentation:

```sh
cmake -S benchmarks -B build-bench -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench --target urlparser_benchmarks_report
# -> benchmarks/results/report.html
```

The two tables below are the original measurements from March 2022.

#### Extract From Host

Tests were run on a file containing 10 million random domains from various top-level domains (Mar. 13rd 2022)

| Library                                                             | Function                  | Time   |
| ------------------------------------------------------------------- | ------------------------- | ------ |
| [liburlparser](https://github.com/mohammadraziei/liburlparser)      | liburlparser.Hostname     | 1.12s  |
| [PyDomainExtractor](https://github.com/Intsights/PyDomainExtractor) | pydomainextractor.extract | 1.50s  |
| [publicsuffix2](https://github.com/nexb/python-publicsuffix2)       | publicsuffix2.get_sld     | 9.92s  |
| [tldextract](https://github.com/john-kurkowski/tldextract)          | \_\_call\_\_              | 29.23s |
| [tld](https://github.com/barseghyanartur/tld)                       | tld.parse_tld             | 34.48s |

#### Extract From URL

The test was conducted on a file containing 1 million random urls (Mar. 13rd 2022)

| Library                                                             | Function                           | Time   |
| ------------------------------------------------------------------- | ---------------------------------- | ------ |
| [liburlparser](https://github.com/mohammadraziei/liburlparser)      | liburlparser.Hostname.from_url     | 2.10s  |
| [PyDomainExtractor](https://github.com/Intsights/PyDomainExtractor) | pydomainextractor.extract_from_url | 2.24s  |
| [publicsuffix2](https://github.com/nexb/python-publicsuffix2)       | publicsuffix2.get_sld              | 10.84s |
| [tldextract](https://github.com/john-kurkowski/tldextract)          | \_\_call\_\_                       | 36.04s |
| [tld](https://github.com/barseghyanartur/tld)                       | tld.parse_tld                      | 57.87s |

## License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.

## Stats

[![Stars](https://starchart.cc/mohammadraziei/liburlparser.svg?variant=adaptive)](https://starchart.cc/mohammadraziei/liburlparser)

## Contact

<!-- Gal Ben David - gal@intsights.com -->

Project Link:

- [https://github.com/mohammadraziei/liburlparser](https://github.com/mohammadraziei/liburlparser)
- [https://pypi.org/project/liburlparser](https://pypi.org/project/liburlparser)
- [Documentation](https://mohammadraziei.github.io/liburlparser/) and [Benchmarks](https://mohammadraziei.github.io/liburlparser/benchmarks/)

[license-shield]: https://img.shields.io/github/license/othneildrew/Best-README-Template.svg?style=flat-square
