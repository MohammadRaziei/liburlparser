# Performance

liburlparser is designed to be extremely fast and efficient. This page provides performance benchmarks and tips for optimizing your code when working with liburlparser.

## Benchmarks

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

## Performance Optimization Tips

### 1. Choose the Right Method

liburlparser provides several methods for extracting domain information, each with different performance characteristics:

```python
from liburlparser import Url, Host
import time

url_str = "https://mail.google.com/about"

# Method 1: Full Url object creation (slowest, but provides all URL components)
start = time.time()
url = Url(url_str)
domain = url.domain
print(f"Method 1 time: {time.time() - start:.6f}s")

# Method 2: Host object from URL (faster, provides all host components)
start = time.time()
host = Host.from_url(url_str)
domain = host.domain
print(f"Method 2 time: {time.time() - start:.6f}s")

# Method 3: Extract host string only (very fast)
start = time.time()
host_str = Url.extract_host(url_str)
print(f"Method 3 time: {time.time() - start:.6f}s")

# Method 4: Extract components directly (fastest for domain extraction)
start = time.time()
components = Host.extract_from_url(url_str)
domain = components["domain"]
print(f"Method 4 time: {time.time() - start:.6f}s")
```

### 2. Batch Processing

For processing large numbers of URLs, use the fastest method appropriate for your needs:

```python
from liburlparser import Host
import time

# Sample list of URLs
urls = ["https://example.com", "https://google.com", "https://github.com"] * 1000

# Method 1: Creating full Host objects
start = time.time()
domains1 = []
for url in urls:
    host = Host.from_url(url)
    domains1.append(host.domain)
print(f"Method 1 time: {time.time() - start:.4f}s")

# Method 2: Using extract_from_url (faster)
start = time.time()
domains2 = []
for url in urls:
    info = Host.extract_from_url(url)
    domains2.append(info["domain"])
print(f"Method 2 time: {time.time() - start:.4f}s")
```

### 3. Memory Optimization

If you're processing millions of URLs and memory usage is a concern, use the extraction methods instead of creating full objects:

```python
# Memory-efficient processing
from liburlparser import Host

def process_url_file(input_file, output_file):
    with open(input_file, 'r') as infile, open(output_file, 'w') as outfile:
        for line in infile:
            url = line.strip()
            try:
                # Use extract_from_url instead of creating Host objects
                info = Host.extract_from_url(url)
                outfile.write(f"{url},{info['domain']},{info['suffix']}\n")
            except Exception:
                outfile.write(f"{url},error,error\n")
```

### 4. Profiling Your Code

You can use Python's built-in profiling tools to identify performance bottlenecks:

```python
import cProfile
from liburlparser import Host

def process_urls(urls):
    results = []
    for url in urls:
        info = Host.extract_from_url(url)
        results.append(info["domain"])
    return results

# Generate sample data
urls = ["https://example.com", "https://google.com", "https://github.com"] * 1000

# Profile the function
cProfile.run('process_urls(urls)')
```

## Comparison with Other Libraries

liburlparser is significantly faster than other Python domain extraction libraries because:

1. It's implemented in C++ with Python bindings
2. It uses efficient data structures for the Public Suffix List
3. It provides specialized methods for different use cases

If you're migrating from another library, here's how liburlparser compares:

```python
# tldextract
import tldextract
extracted = tldextract.extract("mail.google.com")
domain = extracted.domain
suffix = extracted.suffix
subdomain = extracted.subdomain

# liburlparser equivalent (much faster)
from liburlparser import Host
host = Host("mail.google.com")
domain = host.domain
suffix = host.suffix
subdomain = host.subdomain

# publicsuffix2
from publicsuffix2 import get_sld
domain = get_sld("mail.google.com")

# liburlparser equivalent
from liburlparser import Host
domain = Host("mail.google.com").domain

# pydomainextractor
import pydomainextractor
extractor = pydomainextractor.DomainExtractor()
result = extractor.extract("mail.google.com")

# liburlparser equivalent
from liburlparser import Host
result = Host.extract("mail.google.com")
```