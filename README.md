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

![license](https://img.shields.io/badge/MIT-License-green)
![Python]( https://img.shields.io/badge/Python-3.8%20%7C%203.9%20%7C%203.10%20%7C%203.11-blue)
![Python]( https://img.shields.io/badge/C++-17-yellow)
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

### Features
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


## Setup
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
```sh
pip install liburlparser
```
Or
```sh
pip install git+https://github.com/mohammadraziei/liburlparser
```
Or
```sh
git clone https://github.com/mohammadraziei/liburlparser
pip install ./liburlparser
```

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
#include "liburlparser"
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

| Library  | Function | Time |
| ------------- | ------------- | ------------- |
| [liburlparser](https://github.com/Intsights/PyDomainExtractor) | liburlparser.Host.from_url | 2.20s |
| [PyDomainExtractor](https://github.com/Intsights/PyDomainExtractor) | pydomainextractor.extract_from_url | 2.24s |
| [publicsuffix2](https://github.com/nexb/python-publicsuffix2) | publicsuffix2.get_sld | 10.84s |
| [tldextract](https://github.com/john-kurkowski/tldextract) | \_\_call\_\_ | 36.04s |
| [tld](https://github.com/barseghyanartur/tld) | tld.parse_tld | 57.87s |



## License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.


## Contact

<!-- Gal Ben David - gal@intsights.com -->

Project Link: [https://github.com/mohammadraziei/liburlparser](https://github.com/mohammadraziei/liburlparser)




[license-shield]: https://img.shields.io/github/license/othneildrew/Best-README-Template.svg?style=flat-square