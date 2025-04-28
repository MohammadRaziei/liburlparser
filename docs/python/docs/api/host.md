# Host Class API Reference

The `Host` class represents a hostname and provides methods to parse and extract information from it.

## Import

```python
from liburlparser import Host
```

## Constructor

```python
Host(hoststr, ignore_www=False)
```

### Parameters

- `hoststr` (str): The hostname to parse
- `ignore_www` (bool, optional): Whether to ignore "www" in the subdomain. Default is `False`.

### Example

```python
# Create a Host object
host = Host("mail.google.com")

# Create a Host object, ignoring "www" if present
host = Host("www.example.com", ignore_www=True)
```

## Class Methods

### from_url

```python
@classmethod
def from_url(cls, urlstr, ignore_www=False)
```

Create a Host object from a URL string.

#### Parameters

- `urlstr` (str): The URL string to parse
- `ignore_www` (bool, optional): Whether to ignore "www" in the subdomain. Default is `False`.

#### Returns

- A new `Host` object

#### Example

```python
host = Host.from_url("https://mail.google.com/about")
print(host.domain)  # google
```

### extract_from_url

```python
@classmethod
def extract_from_url(cls, urlstr)
```

Extract domain components from a URL string.

#### Parameters

- `urlstr` (str): The URL string to parse

#### Returns

- A dictionary with keys 'subdomain', 'domain', and 'suffix'

#### Example

```python
info = Host.extract_from_url("https://mail.google.com/about")
print(info)  # {'subdomain': 'mail', 'domain': 'google', 'suffix': 'com'}
```

### extract

```python
@classmethod
def extract(cls, hoststr)
```

Extract domain components from a host string.

#### Parameters

- `hoststr` (str): The hostname to parse

#### Returns

- A dictionary with keys 'subdomain', 'domain', and 'suffix'

#### Example

```python
info = Host.extract("mail.google.com")
print(info)  # {'subdomain': 'mail', 'domain': 'google', 'suffix': 'com'}
```

### load_psl_from_path

```python
@classmethod
def load_psl_from_path(cls, filepath)
```

Load the Public Suffix List from a file.

#### Parameters

- `filepath` (str): Path to the PSL file

#### Example

```python
Host.load_psl_from_path("/path/to/public_suffix_list.dat")
```

### load_psl_from_string

```python
@classmethod
def load_psl_from_string(cls, string)
```

Load the Public Suffix List from a string.

#### Parameters

- `string` (str): The PSL content as a string

#### Example

```python
with open("/path/to/public_suffix_list.dat", "r") as f:
    psl_content = f.read()
    Host.load_psl_from_string(psl_content)
```

### is_psl_loaded

```python
@classmethod
def is_psl_loaded(cls)
```

Check if the Public Suffix List is loaded.

#### Returns

- `True` if loaded, `False` otherwise

#### Example

```python
is_loaded = Host.is_psl_loaded()
print(f"PSL loaded: {is_loaded}")
```

### removeWWW

```python
@classmethod
def removeWWW(cls, hoststr)
```

Remove "www." from the beginning of a hostname.

#### Parameters

- `hoststr` (str): The hostname

#### Returns

- The hostname without "www."

#### Example

```python
host_without_www = Host.removeWWW("www.example.com")
print(host_without_www)  # example.com
```

## Properties

### subdomain

The subdomain part of the hostname.

```python
host = Host("mail.google.com")
print(host.subdomain)  # mail
```

### domain

The domain part of the hostname.

```python
host = Host("mail.google.com")
print(host.domain)  # google
```

### domain_name

The domain name (same as domain).

```python
host = Host("mail.google.com")
print(host.domain_name)  # google
```

### fulldomain

The full domain (domain + suffix).

```python
host = Host("mail.google.com")
print(host.fulldomain)  # google.com
```

### suffix

The suffix part of the hostname.

```python
host = Host("mail.google.com")
print(host.suffix)  # com
```

## Methods

### to_dict

Convert the Host object to a dictionary.

```python
host.to_dict()
```

#### Returns

- A dictionary with keys 'str', 'subdomain', 'domain', 'domain_name', and 'suffix'

#### Example

```python
host = Host("mail.google.com")
host_dict = host.to_dict()
print(host_dict)
# {'str': 'mail.google.com', 'subdomain': 'mail', 'domain': 'google', 'domain_name': 'google', 'suffix': 'com'}
```

### to_json

Convert the Host object to a JSON string.

```python
host = Host("mail.google.com")
host_json = host.to_json()
print(host_json)
# {"str": "mail.google.com", "subdomain": "mail", "domain": "google", "domain_name": "google", "suffix": "com"}
```

### __str__

Get the string representation of the hostname.

```python
str(host)
```

#### Returns

- The hostname as a string

#### Example

```python
host = Host("mail.google.com")
print(str(host))  # mail.google.com
```

### __repr__

Get the representation of the Host object.

```python
repr(host)
```

#### Returns

- A string like `<Host :'example.com'>`

#### Example

```python
host = Host("mail.google.com")
print(repr(host))  # <Host :'mail.google.com'>
```

## Complete Example

Here's a complete example that demonstrates the Host class functionality:

```python
from liburlparser import Host

def analyze_host(host_str):
    # Parse the host
    host = Host(host_str)
    
    # Print host components
    print(f"Full host: {host}")
    print(f"Subdomain: {host.subdomain}")
    print(f"Domain: {host.domain}")
    print(f"Suffix: {host.suffix}")
    print(f"Full domain: {host.fulldomain}")
    
    # Convert to dictionary and JSON
    print(f"Dictionary: {host.to_dict()}")
    print(f"JSON: {host.to_json()}")

# Test with different hostnames
hosts = [
    "example.com",
    "www.example.com",
    "mail.google.com",
    "blog.example.co.uk",
    "a.b.c.example.org"
]

for host_str in hosts:
    print(f"\nAnalyzing: {host_str}")
    analyze_host(host_str)
```

## Handling Complex Domains

The Host class can handle various domain structures:

```python
# Standard domains
host = Host("example.com")
print(f"Domain: {host.domain}, Suffix: {host.suffix}")  # Domain: example, Suffix: com

# Domains with subdomains
host = Host("www.mail.example.com")
print(f"Subdomain: {host.subdomain}, Domain: {host.domain}, Suffix: {host.suffix}")
# Subdomain: www.mail, Domain: example, Suffix: com

# Country-specific domains
host = Host("example.co.uk")
print(f"Domain: {host.domain}, Suffix: {host.suffix}")  # Domain: example, Suffix: co.uk

# Domains with unusual TLDs
host = Host("example.museum")
print(f"Domain: {host.domain}, Suffix: {host.suffix}")  # Domain: example, Suffix: museum

# IDN domains
host = Host("例子.测试")
print(f"Domain: {host.domain}, Suffix: {host.suffix}")  # Domain: 例子, Suffix: 测试
```

## Error Handling

It's good practice to handle potential errors when parsing hosts:

```python
def safe_parse_host(host_str):
    try:
        host = Host(host_str)
        return {
            'success': True,
            'host': host,
            'domain': host.domain,
            'suffix': host.suffix,
            'subdomain': host.subdomain
        }
    except Exception as e:
        return {
            'success': False,
            'error': str(e),
            'host_str': host_str
        }

# Test with valid and invalid hosts
hosts = [
    "example.com",
    "mail.google.com",
    "",  # Empty string
    "invalid"  # No suffix
]

for host_str in hosts:
    result = safe_parse_host(host_str)
    if result['success']:
        print(f"Successfully parsed: {host_str} → Domain: {result['domain']}")
    else:
        print(f"Failed to parse: {host_str} → Error: {result['error']}")
```