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
![Python](https://img.shields.io/badge/Python-3.8%20%7C%203.9%20%7C%203.10%20%7C%203.11-blue)
![Cpp](https://img.shields.io/badge/C++-17-blue)


[![GitHub release](https://img.shields.io/github/release/mohammadraziei/liburlparser?include_prereleases=&sort=semver&color=purple)](https://github.com/mohammadraziei/liburlparser/releases/)
[![License](https://img.shields.io/badge/License-MIT-purple)](#license)
[![issues - liburlparser](https://img.shields.io/github/issues/mohammadraziei/liburlparser)](https://github.com/mohammadraziei/liburlparser/issues)


[![SonarCloud](https://sonarcloud.io/images/project_badges/sonarcloud-white.svg)](https://sonarcloud.io/summary/new_code?id=MohammadRaziei_liburlparser)

[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=MohammadRaziei_liburlparser&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=MohammadRaziei_liburlparser)
[![snyk.io](https://snyk.io/advisor/python/liburlparser/badge.svg)](https://snyk.io/advisor/python/liburlparser)

[//]: # ([![View site - GH Pages]&#40;https://img.shields.io/badge/View_site-GH_Pages-2ea44f?style=for-the-badge&#41;]&#40;https://mohammadraziei.github.io/liburlparser/&#41;)



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
   - The library provides two separate classes: `Url` and `Host`.
   - This separation allows for cleaner and more organized code when dealing with URLs.

3. **Public Suffix List Support**:
   - liburlparser supports known combinatorial suffixes (e.g., "ac.ir") using the public_suffix_list.
   - It can also handle unknown suffixes (e.g., "comm" in "google.comm").

4. **Automatic Public Suffix List Updates**:
   - Before each build and deployment, liburlparser updates the public_suffix_list automatically.

5. **Host Properties**:
   - The `Host` class includes properties such as subdomain, domain, domain name, and suffix.

6. **URL Properties**:
   - The `Url` class provides properties like protocol, userinfo, host (and all host properties), port, path, query parameters, and fragment.


<!--
* Multiple programming language supported such as `Python`, `C++` and `Shell`
* Intuitive interface and identical in C++ and Python
* Provide two seperated class Url and Host for the purpose of clean code
* Also support [public_suffix_list](https://publicsuffix.org/list/public_suffix_list.dat) for known combinatorial suffix such as "ac.ir"
* Support unknown suffix like "google.comm" (it detect "comm" as suffix)
* Update public_suffix_list automatically before each build and deploy
* Host properties:
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

from liburlparser import Url, Host
help(Url)
help(Host)
```

parse url and host
```python
from liburlparser import Url, Host
## parse url:
url = Url("https://ee.aut.ac.ir/#id") # parse all part of url
print(url, url.suffix, url.domain, url.fragment, url.host, url.to_dict(), url.to_json())
## parse host
host = url.host # ee.aut.ac.ir
# or
host = Host("ee.aut.ac.ir")
# or 
host = Host.from_url("https://ee.aut.ac.ir/#id") # the fastest way for parsing host from url
# all of these methods return an object of Host class which already parse the host part of url 
print(host, host.domain, host.suffix, host.to_dict(), host.to_json())
```
Also there is some helping api to get better performance for some small tasks

```python
# if you need to extract the host of url as a string without any parsing 
host_str = Url.extract_host("https://ee.aut.ac.ir/about") # very fast
```
if you are fan of  `pydomainextractor`, there is some interface similar to it
```python
import pydomainextractor
extractor = pydomainextractor.DomainExtractor()
extractor.extract("ee.aut.ac.ir") # from host
extractor.extract_from_url("https://ee.aut.ac.ir/about") # from url

# alternatively you can use:
from liburlparser import Host
Host.extract("ee.aut.ac.ir") # from host
Host.extract_from_url("https://ee.aut.ac.ir/about") # from url
# you can see there is the same api
```

### C++
there is some examples in [examples](https://github.com/MohammadRaziei/liburlparser/tree/master/examples) folder

```c++
#include "urlparser.h"
...
/// for parsing url
TLD::Url url("https://ee.aut.ac.ir/about");
std::string domain = url.domain(); // also for subdomain, port, params, ...
/// for parsing host
TLD::Host host("ee.aut.ac.ir");
// or
TLD::Host host = url.host();
// or
TLD::Host host = TLD::Host::fromUrl("https://ee.aut.ac.ir/about");
```
you can see all methods in python we can use in c++ very easily



## Installation
### C++:

#### build steps:
```sh
git clone https://github.com/mohammadraziei/liburlparser
mkdir -p build; cd build
cmake ..
# Build the project:
make
# [Optional] run tests:
make test
# [Optional] make documents:
make docs
# [Optional] Run examples:
./example
# Make install
sudo make install
```



### Python and Command Line:
Be aware that it required `python>=3.8`
#### Installation
###### pip by [pypi](https://pypi.org/project/liburlparser/)
```sh
pip install liburlparser
```
if you want to use psl.update to update the public suffix list, you must install the `online` version
```sh
pip install "liburlparser[online]"
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


#### Extract From Host

Tests were run on a file containing 10 million random domains from various top-level domains (Mar. 13rd 2022)

| Library  | Function | Time |
| ------------- | ------------- | ------------- |
| [liburlparser](https://github.com/mohammadraziei/liburlparser) | liburlparser.Host | 1.12s |
| [PyDomainExtractor](https://github.com/Intsights/PyDomainExtractor) | pydomainextractor.extract | 1.50s |
| [publicsuffix2](https://github.com/nexb/python-publicsuffix2) | publicsuffix2.get_sld | 9.92s |
| [tldextract](https://github.com/john-kurkowski/tldextract) | \_\_call\_\_ | 29.23s |
| [tld](https://github.com/barseghyanartur/tld) | tld.parse_tld | 34.48s |


#### Extract From URL

The test was conducted on a file containing 1 million random urls (Mar. 13rd 2022)

| Library                                                             | Function | Time   |
|---------------------------------------------------------------------| ------------- |--------|
| [liburlparser](https://github.com/mohammadraziei/liburlparser)      | liburlparser.Host.from_url | 2.10s  |
| [PyDomainExtractor](https://github.com/Intsights/PyDomainExtractor) | pydomainextractor.extract_from_url | 2.24s  |
| [publicsuffix2](https://github.com/nexb/python-publicsuffix2)       | publicsuffix2.get_sld | 10.84s |
| [tldextract](https://github.com/john-kurkowski/tldextract)          | \_\_call\_\_ | 36.04s |
| [tld](https://github.com/barseghyanartur/tld)                       | tld.parse_tld | 57.87s |



## License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.


## Stats
[![Stars](https://starchart.cc/mohammadraziei/liburlparser.svg?variant=adaptive)](https://starchart.cc/mohammadraziei/liburlparser)

## Contact

<!-- Gal Ben David - gal@intsights.com -->

Project Link:
- [https://github.com/mohammadraziei/liburlparser](https://github.com/mohammadraziei/liburlparser)
- [https://pypi.org/project/liburlparser](https://pypi.org/project/liburlparser)



[license-shield]: https://img.shields.io/github/license/othneildrew/Best-README-Template.svg?style=flat-square
