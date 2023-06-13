<p align="center">
  <a href="https://github.com/mohammadraziei/liburlparser">
    <img src="docs/images/logo/liburlparser-logo-1.svg" alt="Logo">
  </a>
  <h3 align="center">
    Fastest domain extractor library written in C++ with python binding.
  </h3>
</p>

![license](https://img.shields.io/badge/MIT-License-blue)
![Python]( https://img.shields.io/badge/Python-3.7%20%7C%203.8%20%7C%203.9%20%7C%203.10-blue)
<!--
![Build](https://github.com/Intsights/PyDomainExtractor/workflows/Build/badge.svg)
[![PyPi](https://img.shields.io/pypi/v/PyDomainExtractor.svg)](https://pypi.org/project/PyDomainExtractor/)
-->
## Table of Contents

- [Table of Contents](#table-of-contents)
- [About The Project](#about-the-project)
  - [Built With](#built-with)
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


## About The Project


### Performance


#### Extract From Domain

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

## C++:

### build steps:
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



## Python:
### Installation

```sh
pip install git+https://github.com/mohammadraziei/liburlparser
```
Or
```sh
git clone https://github.com/mohammadraziei/liburlparser
pip install ./liburlparser
```

## Usage




### URL Extraction




## License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.


## Contact

<!-- Gal Ben David - gal@intsights.com -->

Project Link: [https://github.com/mohammadraziei/liburlparser](https://github.com/mohammadraziei/liburlparser)




[license-shield]: https://img.shields.io/github/license/othneildrew/Best-README-Template.svg?style=flat-square