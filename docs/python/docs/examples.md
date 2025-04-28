# Examples

This page provides practical examples of using liburlparser in Python for various URL parsing tasks.

## Basic URL Parsing

```python
from liburlparser import Url

# Parse a simple URL
url = Url("https://www.example.com")
print(f"Domain: {url.domain}")        # Domain: example
print(f"Suffix: {url.suffix}")        # Suffix: com
print(f"Subdomain: {url.subdomain}")  # Subdomain: www

# Parse a more complex URL
url = Url("https://user:pass@mail.google.com:8080/path/to/page?q=test&lang=en#section")
print(f"Protocol: {url.protocol}")    # Protocol: https
print(f"Userinfo: {url.userinfo}")    # Userinfo: user:pass
print(f"Domain: {url.domain}")        # Domain: google
print(f"Subdomain: {url.subdomain}")  # Subdomain: mail
print(f"Port: {url.port}")            # Port: 8080
print(f"Query: {url.query}")          # Query: q=test&lang=en
print(f"Fragment: {url.fragment}")    # Fragment: section
```

## Host Parsing

```python
from liburlparser import Host

# Parse a simple hostname
host = Host("example.com")
print(f"Domain: {host.domain}")        # Domain: example
print(f"Suffix: {host.suffix}")        # Suffix: com
print(f"Subdomain: {host.subdomain}")  # Subdomain: (empty string)

# Parse a hostname with subdomain
host = Host("blog.example.co.uk")
print(f"Domain: {host.domain}")        # Domain: example
print(f"Suffix: {host.suffix}")        # Suffix: co.uk
print(f"Subdomain: {host.subdomain}")  # Subdomain: blog

# Parse a hostname with multiple subdomain levels
host = Host("a.b.c.example.com")
print(f"Domain: {host.domain}")        # Domain: example
print(f"Suffix: {host.suffix}")        # Suffix: com
print(f"Subdomain: {host.subdomain}")  # Subdomain: a.b.c
```

## Extracting Host from URL

```python
from liburlparser import Url, Host

# Method 1: Get the host from a URL object
url = Url("https://mail.google.com/about")
host = url.host
print(f"Host: {host}")  # Host: <Host :'mail.google.com'>

# Method 2: Use the Host.from_url static method
host = Host.from_url("https://mail.google.com/about")
print(f"Domain: {host.domain}")  # Domain: google

# Method 3: Just extract the host string (fastest)
host_str = Url.extract_host("https://mail.google.com/about")
print(f"Host string: {host_str}")  # Host string: mail.google.com
```

## Ignoring "www" Subdomain

```python
from liburlparser import Url, Host

# Default behavior
host = Host("www.example.com")
print(f"Subdomain: '{host.subdomain}'")  # Subdomain: 'www'
print(f"Domain: {host.domain}")          # Domain: example

# Ignore www
host = Host("www.example.com", ignore_www=True)
print(f"Subdomain: '{host.subdomain}'")  # Subdomain: ''
print(f"Domain: {host.domain}")          # Domain: example

# Same for URLs
url = Url("https://www.example.com/about", ignore_www=True)
print(f"Subdomain: '{url.subdomain}'")   # Subdomain: ''
print(f"Domain: {url.domain}")           # Domain: example
```

## Converting to Dictionary or JSON

```python
from liburlparser import Url, Host
import json

# URL to dictionary
url = Url("https://mail.google.com/about?q=test#section")
url_dict = url.to_dict()
print(json.dumps(url_dict, indent=2))
# Output:
# {
#   "str": "https://mail.google.com/about?q=test#section",
#   "protocol": "https",
#   "userinfo": "",
#   "host": {
#     "str": "mail.google.com",
#     "subdomain": "mail",
#     "domain": "google",
#     "domain_name": "google",
#     "suffix": "com"
#   },
#   "port": 0,
#   "query": "q=test",
#   "fragment": "section"
# }

# URL to JSON
url_json = url.to_json()
print(url_json)
# Output: {"str": "https://mail.google.com/about?q=test#section", "protocol": "https", "userinfo": "", "host": {"str": "mail.google.com", "subdomain": "mail", "domain": "google", "domain_name": "google", "suffix": "com"}, "port": 0, "query": "q=test", "fragment": "section"}

# Host to dictionary
host = Host("mail.google.com")
host_dict = host.to_dict()
print(json.dumps(host_dict, indent=2))
# Output:
# {
#   "str": "mail.google.com",
#   "subdomain": "mail",
#   "domain": "google",
#   "domain_name": "google",
#   "suffix": "com"
# }

# Host to JSON
host_json = host.to_json()
print(host_json)
# Output: {"str": "mail.google.com", "subdomain": "mail", "domain": "google", "domain_name": "google", "suffix": "com"}
```

## Quick Domain Extraction

```python
from liburlparser import Host

# From a host string
result = Host.extract("mail.google.com")
print(result)  # {'suffix': 'com', 'domain': 'google', 'subdomain': 'mail'}

# From a URL string
result = Host.extract_from_url("https://mail.google.com/about")
print(result)  # {'suffix': 'com', 'domain': 'google', 'subdomain': 'mail'}
```

## Batch Processing URLs

```python
from liburlparser import Host
import csv

def extract_domains_from_csv(input_file, url_column, output_file):
    with open(input_file, 'r') as infile, open(output_file, 'w', newline='') as outfile:
        reader = csv.DictReader(infile)
        fieldnames = list(reader.fieldnames) + ['domain', 'suffix', 'subdomain']
        writer = csv.DictWriter(outfile, fieldnames=fieldnames)
        writer.writeheader()
        
        for row in reader:
            url = row[url_column]
            try:
                # Use the fast extract_from_url method
                domain_info = Host.extract_from_url(url)
                row['domain'] = domain_info['domain']
                row['suffix'] = domain_info['suffix']
                row['subdomain'] = domain_info['subdomain']
            except Exception:
                row['domain'] = ''
                row['suffix'] = ''
                row['subdomain'] = ''
            
            writer.writerow(row)

# Example usage
# extract_domains_from_csv('urls.csv', 'url_column', 'output.csv')
```

## Error Handling

```python
from liburlparser import Url, Host

def safe_parse_url(url_str):
    try:
        url = Url(url_str)
        return url
    except Exception as e:
        print(f"Error parsing URL '{url_str}': {e}")
        return None

def safe_parse_host(host_str):
    try:
        host = Host(host_str)
        return host
    except Exception as e:
        print(f"Error parsing host '{host_str}': {e}")
        return None

# Test with valid and invalid URLs
urls = [
    "https://example.com",
    "invalid://example.com",
    "https://example.com:invalid",
    "not a url"
]

for url_str in urls:
    url = safe_parse_url(url_str)
    if url:
        print(f"Successfully parsed: {url.domain}")
```

## Working with International Domain Names (IDNs)

```python
from liburlparser import Url, Host

# Parse an IDN
url = Url("https://例子.测试")
print(f"Domain: {url.domain}")  # Domain: 例子
print(f"Suffix: {url.suffix}")  # Suffix: 测试

host = Host("例子.测试")
print(f"Domain: {host.domain}")  # Domain: 例子
print(f"Suffix: {host.suffix}")  # Suffix: 测试
```