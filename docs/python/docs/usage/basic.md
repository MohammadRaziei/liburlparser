# Basic Usage

This guide covers the fundamental operations of liburlparser in Python.

## Importing the Library

```python
from liburlparser import Url, Host
```

## Parsing URLs

The `Url` class is used to parse complete URLs:

```python
# Create a URL object
url = Url("https://mail.google.com/about?q=test#section")

# Access URL components
print(url)                # <Url :'https://mail.google.com/about?q=test#section'>
print(url.protocol)       # https
print(url.host)           # <Host :'mail.google.com'>
print(url.subdomain)      # mail
print(url.domain)         # google
print(url.domain_name)    # google
print(url.suffix)         # com
print(url.port)           # 0 (default)
print(url.query)          # q=test
print(url.fragment)       # section
```

## Parsing Hosts

The `Host` class is used to parse hostnames:

```python
# Create a Host object directly
host = Host("mail.google.com")

# Access host components
print(host)               # <Host :'mail.google.com'>
print(host.subdomain)     # mail
print(host.domain)        # google
print(host.domain_name)   # google
print(host.suffix)        # com
```

## Getting Host from URL

There are multiple ways to extract host information from a URL:

```python
# Method 1: Get the host from a URL object
url = Url("https://mail.google.com/about")
host = url.host

# Method 2: Use the Host.from_url static method
host = Host.from_url("https://mail.google.com/about")

# Method 3: Just extract the host string (fastest)
host_str = Url.extract_host("https://mail.google.com/about")
print(host_str)  # mail.google.com
```

## Converting to Dictionary or JSON

Both `Url` and `Host` objects can be converted to dictionaries or JSON strings:

```python
# URL to dictionary
url = Url("https://mail.google.com/about?q=test#section")
url_dict = url.to_dict()
print(url_dict)
# Output: {'str': 'https://mail.google.com/about?q=test#section', 'protocol': 'https', 'userinfo': '', 'host': {'str': 'mail.google.com', 'subdomain': 'mail', 'domain': 'google', 'domain_name': 'google', 'suffix': 'com'}, 'port': 0, 'query': 'q=test', 'fragment': 'section'}

# URL to JSON
url_json = url.to_json()
print(url_json)
# Output: {"str": "https://mail.google.com/about?q=test#section", "protocol": "https", "userinfo": "", "host": {"str": "mail.google.com", "subdomain": "mail", "domain": "google", "domain_name": "google", "suffix": "com"}, "port": 0, "query": "q=test", "fragment": "section"}

# Host to dictionary
host = Host("mail.google.com")
host_dict = host.to_dict()
print(host_dict)
# Output: {'str': 'mail.google.com', 'subdomain': 'mail', 'domain': 'google', 'domain_name': 'google', 'suffix': 'com'}

# Host to JSON
host_json = host.to_json()
print(host_json)
# Output: {"str": "mail.google.com", "subdomain": "mail", "domain": "google", "domain_name": "google", "suffix": "com"}
```

## Quick Domain Extraction

If you only need the domain components without creating full objects:

```python
# From a host string
result = Host.extract("mail.google.com")
print(result)  # {'suffix': 'com', 'domain': 'google', 'subdomain': 'mail'}

# From a URL string
result = Host.extract_from_url("https://mail.google.com/about")
print(result)  # {'suffix': 'com', 'domain': 'google', 'subdomain': 'mail'}
```

## Ignoring "www" Subdomain

You can choose to ignore the "www" subdomain when parsing:

```python
# Default behavior
host = Host("www.example.com")
print(host.subdomain)  # www
print(host.domain)     # example

# Ignore www
host = Host("www.example.com", ignore_www=True)
print(host.subdomain)  # (empty string)
print(host.domain)     # example

# Same for URLs
url = Url("https://www.example.com/about", ignore_www=True)
print(url.subdomain)   # (empty string)
print(url.domain)      # example
```

## Removing "www" from a Host String

```python
host_str = Host.removeWWW("www.example.com")
print(host_str)  # example.com
```

## Complete Example

Here's a complete example that demonstrates parsing a URL and accessing its components:

```python
from liburlparser import Url, Host

def analyze_url(url_str):
    # Parse the URL
    url = Url(url_str)
    
    # Print URL components
    print(f"Full URL: {url}")
    print(f"Protocol: {url.protocol}")
    print(f"Host: {url.host}")
    print(f"Subdomain: {url.subdomain}")
    print(f"Domain: {url.domain}")
    print(f"Suffix: {url.suffix}")
    print(f"Port: {url.port}")
    print(f"Query: {url.query}")
    print(f"Fragment: {url.fragment}")
    
    # Convert to JSON
    print(f"JSON: {url.to_json()}")

# Test with a sample URL
analyze_url("https://mail.google.com/about?q=test#section")
```