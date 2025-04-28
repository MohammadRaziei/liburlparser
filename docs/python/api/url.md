# Url Class API Reference

The `Url` class is used to parse and extract information from URLs.

## Import

```python
from liburlparser import Url
```

## Constructor

```python
Url(urlstr, ignore_www=False)
```

### Parameters

- `urlstr` (str): The URL string to parse
- `ignore_www` (bool, optional): Whether to ignore "www" in the subdomain. Default is `False`.

### Example

```python
# Create a Url object
url = Url("https://mail.google.com/about")

# Create a Url object, ignoring "www" if present
url = Url("https://www.example.com/about", ignore_www=True)
```

## Class Methods

### extract_host

```python
@classmethod
def extract_host(cls, urlstr)
```

Extract the host part from a URL string.

#### Parameters

- `urlstr` (str): The URL string to parse

#### Returns

- The hostname as a string

#### Example

```python
host_str = Url.extract_host("https://mail.google.com/about")
print(host_str)  # mail.google.com
```

## Properties

### protocol

The protocol part of the URL (e.g., "http", "https").

```python
url = Url("https://mail.google.com/about")
print(url.protocol)  # https
```

### userinfo

The userinfo part of the URL (username and password).

```python
url = Url("https://user:pass@example.com")
print(url.userinfo)  # user:pass
```

### host

The host part of the URL as a `Host` object.

```python
url = Url("https://mail.google.com/about")
print(url.host)  # <Host :'mail.google.com'>
```

### subdomain

The subdomain part of the hostname.

```python
url = Url("https://mail.google.com/about")
print(url.subdomain)  # mail
```

### domain

The domain part of the hostname.

```python
url = Url("https://mail.google.com/about")
print(url.domain)  # google
```

### fulldomain

The full domain (domain + suffix).

```python
url = Url("https://mail.google.com/about")
print(url.fulldomain)  # google.com
```

### domain_name

The domain name (same as domain).

```python
url = Url("https://mail.google.com/about")
print(url.domain_name)  # google
```

### suffix

The suffix part of the hostname.

```python
url = Url("https://mail.google.com/about")
print(url.suffix)  # com
```

### port

The port number (0 if not specified).

```python
url = Url("https://example.com:8080")
print(url.port)  # 8080

url = Url("https://example.com")
print(url.port)  # 0
```

### params

The parameters part of the URL.

```python
url = Url("https://example.com/path;param1=value1;param2=value2")
print(url.params)  # param1=value1;param2=value2
```

### query

The query part of the URL.

```python
url = Url("https://example.com/path?q=test&page=1")
print(url.query)  # q=test&page=1
```

### fragment

The fragment part of the URL.

```python
url = Url("https://example.com/path#section")
print(url.fragment)  # section
```

## Methods

### to_dict

Convert the Url object to a dictionary.

```python
url.to_dict()
```

#### Returns

- A dictionary with keys 'str', 'protocol', 'userinfo', 'host', 'port', 'query', and 'fragment'

#### Example

```python
url = Url("https://mail.google.com/about?q=test#section")
url_dict = url.to_dict()
print(url_dict)
# {'str': 'https://mail.google.com/about?q=test#section', 'protocol': 'https', 'userinfo': '', 'host': {'str': 'mail.google.com', 'subdomain': 'mail', 'domain': 'google', 'domain_name': 'google', 'suffix': 'com'}, 'port': 0, 'query': 'q=test', 'fragment': 'section'}
```

### to_json

Convert the Url object to a JSON string.

```python
url.to_json()
```

#### Returns

- A JSON string representation of the Url object

#### Example

```python
url = Url("https://mail.google.com/about?q=test#section")
url_json = url.to_json()
print(url_json)
# {"str": "https://mail.google.com/about?q=test#section", "protocol": "https", "userinfo": "", "host": {"str": "mail.google.com", "subdomain": "mail", "domain": "google", "domain_name": "google", "suffix": "com"}, "port": 0, "query": "q=test", "fragment": "section"}
```

### __str__

Get the string representation of the URL.

```python
str(url)
```

#### Returns

- The URL as a string

#### Example

```python
url = Url("https://mail.google.com/about")
print(str(url))  # https://mail.google.com/about
```

### __repr__

Get the representation of the Url object.

```python
repr(url)
```

#### Returns

- A string like `<Url :'https://example.com'>`

#### Example

```python
url = Url("https://mail.google.com/about")
print(repr(url))  # <Url :'https://mail.google.com/about'>
```

## Complete Example

```python
from liburlparser import Url

# Parse a complex URL
url = Url("https://user:pass@sub.example.co.uk:8080/path/to/page?q=test&lang=en#section")

# Access all properties
print(f"Full URL: {url}")
print(f"Protocol: {url.protocol}")
print(f"Userinfo: {url.userinfo}")
print(f"Host: {url.host}")
print(f"Subdomain: {url.subdomain}")
print(f"Domain: {url.domain}")
print(f"Suffix: {url.suffix}")
print(f"Full Domain: {url.fulldomain}")
print(f"Port: {url.port}")
print(f"Query: {url.query}")
print(f"Fragment: {url.fragment}")

# Convert to dictionary and JSON
print(f"Dictionary: {url.to_dict()}")
print(f"JSON: {url.to_json()}")
```