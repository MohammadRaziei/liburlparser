# liburlparser for Python

<p align="center">
  <img src="https://github.com/MohammadRaziei/liburlparser/raw/master/docs/images/logo/liburlparser-logo-1.svg" alt="Logo">
  <h3 align="center">
    Fastest domain extractor library for Python
  </h3>
  <h4 align="center">
    Complete library for parsing URLs with Python and Command Line
  </h4>
</p>

## Overview

**liburlparser** is a powerful domain extractor library written in C++ with Python bindings. It provides efficient URL parsing capabilities for Python, making it a valuable tool for projects that involve working with web addresses.

### Key Features

- **High Performance**: Significantly faster than pure Python alternatives
- **Intuitive Interface**: Simple, easy-to-use API
- **Clean Code Design**: Separate `Url` and `Host` classes for organized code
- **Public Suffix List Support**: Handles known combinatorial suffixes (e.g., "ac.ir")
- **Unknown Suffix Support**: Can handle unknown suffixes (e.g., "comm" in "google.comm")
- **Automatic PSL Updates**: Updates the public_suffix_list automatically
- **Comprehensive Properties**: Access all parts of URLs and hosts with simple property access
- **Command Line Interface**: Parse URLs directly from the command line

## Quick Start

```python
from liburlparser import Url, Host

# Parse a URL
url = Url("https://mail.google.com/about")
print(url.domain)  # Output: google
print(url.suffix)  # Output: com
print(url.protocol)  # Output: https

# Parse a host
host = Host("mail.google.com")
print(host.subdomain)  # Output: mail
print(host.domain)  # Output: google
print(host.suffix)  # Output: com
```

## Command Line

```bash
python -m liburlparser --url "https://mail.google.com/about" | jq
python -m liburlparser --host "mail.google.com" | jq
```

## Why liburlparser?

- **Performance**: Significantly faster than other domain extraction libraries
- **Ease of Use**: Simple, intuitive API
- **Comprehensive**: Handles all parts of URLs and hosts
- **Reliable**: Built on the Public Suffix List for accurate domain extraction

Check out the [Installation](installation.md) guide to get started, or dive into the [Basic Usage](usage/basic.md) documentation to learn more.

## Performance Comparison

### Extract From Host

Tests were run on a file containing 10 million random domains from various top-level domains:

| Library  | Function | Time |
| ------------- | ------------- | ------------- |
| **liburlparser** | liburlparser.Host | **1.12s** |
| PyDomainExtractor | pydomainextractor.extract | 1.50s |
| publicsuffix2 | publicsuffix2.get_sld | 9.92s |
| tldextract | \_\_call\_\_ | 29.23s |
| tld | tld.parse_tld | 34.48s |

### Extract From URL

Tests were run on a file containing 1 million random URLs:

| Library  | Function | Time |
| ------------- | ------------- | ------------- |
| **liburlparser** | liburlparser.Host.from_url | **2.10s** |
| PyDomainExtractor | pydomainextractor.extract_from_url | 2.24s |
| publicsuffix2 | publicsuffix2.get_sld | 10.84s |
| tldextract | \_\_call\_\_ | 36.04s |
| tld | tld.parse_tld | 57.87s |