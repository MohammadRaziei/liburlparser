# Advanced Usage

This guide covers more advanced features and techniques for using liburlparser in Python.

## Working with the Public Suffix List (PSL)

liburlparser uses the Public Suffix List to accurately parse domain suffixes. You can interact with the PSL through the global `psl` object:

```python
from liburlparser import psl

# Check if PSL is loaded
print(psl.is_loaded())  # True (by default)

# Get PSL information
print(psl.url)       # URL of the public suffix list
print(psl.filename)  # Filename of the public suffix list

# Load PSL from a custom path
psl.load_from_path("/path/to/custom/public_suffix_list.dat")

# Load PSL from a string
with open("/path/to/custom/public_suffix_list.dat", "r") as f:
    psl_content = f.read()
    psl.load_from_string(psl_content)
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

If you just need to remove "www." from a hostname:

```python
from liburlparser import Host

host_str = Host.removeWWW("www.example.com")
print(host_str)  # example.com
```

## Handling Complex Domains

liburlparser can handle complex domain structures, including multi-part suffixes:

```python
# Standard TLDs
host = Host("example.com")
print(host.domain)  # example
print(host.suffix)  # com

# Country-specific TLDs
host = Host("example.co.uk")
print(host.domain)  # example
print(host.suffix)  # co.uk

# Multi-level subdomains
host = Host("a.b.c.example.com")
print(host.subdomain)  # a.b.c
print(host.domain)     # example
print(host.suffix)     # com
```

## Compatibility with Other Libraries

If you're migrating from pydomainextractor, liburlparser provides compatible methods:

```python
# pydomainextractor style:
import pydomainextractor
extractor = pydomainextractor.DomainExtractor()
result = extractor.extract("mail.google.com")
result = extractor.extract_from_url("https://mail.google.com/about")

# liburlparser equivalent:
from liburlparser import Host
result = Host.extract("mail.google.com")
result = Host.extract_from_url("https://mail.google.com/about")
```

## Error Handling

liburlparser is designed to be robust and handle various edge cases:

```python
# Empty or invalid URLs
try:
    url = Url("")
except Exception as e:
    print(f"Error: {e}")

# URLs without protocols
url = Url("example.com")
print(url.protocol)  # (empty string)
print(url.domain)    # example

# URLs with unusual formats
url = Url("https://user:pass@example.com:8080/path?query=value#fragment")
print(url.userinfo)  # user:pass
print(url.port)      # 8080
print(url.query)     # query=value
```

## Performance Optimization

For high-performance applications, consider these techniques:

1. Use `Host.from_url()` instead of creating a `Url` object when you only need host information:

```python
# Slower (creates full Url object)
url = Url("https://example.com/path")
host = url.host

# Faster (directly extracts host)
host = Host.from_url("https://example.com/path")
```

2. Use `Url.extract_host()` when you only need the host string:

```python
# Fastest (just extracts host string without parsing)
host_str = Url.extract_host("https://example.com/path")
```

3. Use `Host.extract()` or `Host.extract_from_url()` when you only need domain components:

```python
# Fast (returns just the components you need)
components = Host.extract("example.com")
components = Host.extract_from_url("https://example.com/path")
```

## Working with International Domain Names (IDNs)

liburlparser has built-in support for International Domain Names (IDNs):

```python
# Parse IDNs directly
host = Host("例子.测试")
print(host.domain)  # 例子
print(host.suffix)  # 测试

# IDNs in URLs
url = Url("https://例子.测试/path")
print(url.domain)  # 例子
print(url.suffix)  # 测试

# Mixed IDN and ASCII
host = Host("subdomain.例子.测试")
print(host.subdomain)  # subdomain
print(host.domain)     # 例子
print(host.suffix)     # 测试
```

## Batch Processing

For processing large numbers of URLs efficiently:

```python
from liburlparser import Host
import csv

def extract_domains_from_csv(csv_file, url_column, output_file):
    with open(csv_file, 'r', encoding='utf-8') as infile, open(output_file, 'w', encoding='utf-8', newline='') as outfile:
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

## Integration with Other Libraries

### Using with requests

```python
import requests
from liburlparser import Url, Host

def get_domain_info(url_str):
    # Parse the URL first to validate it
    url = Url(url_str)
    
    # Make the request
    response = requests.get(url_str)
    
    # Check if there were redirects
    if response.history:
        final_url = response.url
        final_domain = Host.from_url(final_url).domain
        print(f"Redirected from {url.domain} to {final_domain}")
    
    return {
        'original_domain': url.domain,
        'final_domain': Host.from_url(response.url).domain,
        'status_code': response.status_code
    }

# Example usage
# info = get_domain_info("http://github.com")
```

### Using with pandas

```python
import pandas as pd
from liburlparser import Host

def extract_domains(df, url_column):
    # Create a function to extract domain info
    def get_domain_info(url):
        try:
            info = Host.extract_from_url(url)
            return pd.Series([
                info['domain'],
                info['suffix'],
                info['subdomain']
            ])
        except:
            return pd.Series(['', '', ''])
    
    # Apply the function to the URL column
    domain_info = df[url_column].apply(get_domain_info)
    domain_info.columns = ['domain', 'suffix', 'subdomain']
    
    # Join the results back to the original dataframe
    return pd.concat([df, domain_info], axis=1)

# Example usage
# df = pd.read_csv('urls.csv')
# df_with_domains = extract_domains(df, 'url_column')
```

## Handling Edge Cases

liburlparser is designed to handle various edge cases:

```python
# IP addresses as hosts
url = Url("https://192.168.1.1/path")
print(url.host)  # <Host :'192.168.1.1'>
print(url.domain)  # (empty string, as IPs don't have domains)

# Localhost
url = Url("http://localhost:8080")
print(url.host)  # <Host :'localhost'>
print(url.domain)  # localhost
print(url.suffix)  # (empty string)

# URLs with unusual ports
url = Url("https://example.com:65535/path")
print(url.port)  # 65535

# URLs with empty paths
url = Url("https://example.com")
print(url.path)  # (empty string)

# URLs with query parameters but no path
url = Url("https://example.com?param=value")
print(url.query)  # param=value
```

## Custom PSL Rules

You can create and use a custom Public Suffix List with additional rules:

```python
from liburlparser import psl

# Create a custom PSL with additional rules
custom_psl = """
// Standard PSL rules
com
org
net
// Custom rules for internal domains
internal
local
dev
test
"""

# Load the custom PSL
psl.load_from_string(custom_psl)

# Now parse domains with custom suffixes
host = Host("example.internal")
print(host.domain)  # example
print(host.suffix)  # internal

host = Host("server.dev")
print(host.domain)  # server
print(host.suffix)  # dev
```

## Thread Safety

liburlparser is thread-safe for parsing operations, but PSL loading should be done before spawning threads:

```python
import threading
from liburlparser import Host, psl
import time

# Make sure PSL is loaded before creating threads
if not psl.is_loaded():
    psl.load_from_path("/path/to/public_suffix_list.dat")

def parse_hosts(hosts, results, thread_id):
    for host_str in hosts:
        try:
            host = Host(host_str)
            results.append({
                'thread': thread_id,
                'host': host_str,
                'domain': host.domain,
                'suffix': host.suffix
            })
        except Exception as e:
            results.append({
                'thread': thread_id,
                'host': host_str,
                'error': str(e)
            })

# Example usage
def multi_threaded_parsing(all_hosts, num_threads=4):
    threads = []
    results = []
    
    # Split hosts among threads
    chunk_size = len(all_hosts) // num_threads
    for i in range(num_threads):
        start = i * chunk_size
        end = start + chunk_size if i < num_threads - 1 else len(all_hosts)
        hosts_chunk = all_hosts[start:end]
        
        thread = threading.Thread(
            target=parse_hosts,
            args=(hosts_chunk, results, i)
        )
        threads.append(thread)
        thread.start()
    
    # Wait for all threads to complete
    for thread in threads:
        thread.join()
    
    return results

# Example usage
# hosts = ["example.com", "google.com", "github.com", ...] * 1000
# results = multi_threaded_parsing(hosts, num_threads=8)
```

## Memory Management

For processing very large datasets with minimal memory usage:

```python
from liburlparser import Host
import csv

def process_urls_streaming(input_file, output_file, batch_size=10000):
    with open(input_file, 'r', encoding='utf-8') as infile, open(output_file, 'w', encoding='utf-8', newline='') as outfile:
        reader = csv.reader(infile)
        writer = csv.writer(outfile)
        writer.writerow(['url', 'domain', 'suffix', 'subdomain'])
        
        batch = []
        for i, row in enumerate(reader):
            if i == 0:  # Skip header
                continue
                
            url = row[0]
            batch.append(url)
            
            # Process in batches to avoid memory issues
            if len(batch) >= batch_size:
                process_batch(batch, writer)
                batch = []
        
        # Process remaining URLs
        if batch:
            process_batch(batch, writer)

def process_batch(urls, writer):
    for url in urls:
        try:
            info = Host.extract_from_url(url)
            writer.writerow([
                url,
                info['domain'],
                info['suffix'],
                info['subdomain']
            ])
        except Exception:
            writer.writerow([url, '', '', ''])

# Example usage
# process_urls_streaming('millions_of_urls.csv', 'domains.csv', batch_size=50000)
```

## Advanced URL Parsing

For more complex URL parsing needs:

```python
from liburlparser import Url
import urllib.parse

def parse_query_params(url_str):
    url = Url(url_str)
    query = url.query
    
    # Parse query parameters
    params = urllib.parse.parse_qs(query)
    
    return {
        'url': url_str,
        'domain': url.domain,
        'path': url.path,
        'params': params
    }

# Example
result = parse_query_params("https://example.com/search?q=test&page=1&filter=active")
print(result)
# {
#   'url': 'https://example.com/search?q=test&page=1&filter=active',
#   'domain': 'example',
#   'path': '/search',
#   'params': {'q': ['test'], 'page': ['1'], 'filter': ['active']}
# }
```

## Benchmarking

You can benchmark liburlparser against other libraries:

```python
import time
from liburlparser import Host

# Try to import other libraries
try:
    import tldextract
    has_tldextract = True
except ImportError:
    has_tldextract = False

try:
    import publicsuffix2
    has_publicsuffix2 = True
except ImportError:
    has_publicsuffix2 = False

def benchmark(urls, iterations=3):
    results = {}
    
    # Benchmark liburlparser
    total_time = 0
    for _ in range(iterations):
        start = time.time()
        for url in urls:
            Host.extract_from_url(url)
        total_time += time.time() - start
    results['liburlparser'] = total_time / iterations
    
    # Benchmark tldextract if available
    if has_tldextract:
        total_time = 0
        for _ in range(iterations):
            start = time.time()
            for url in urls:
                tldextract.extract(url)
            total_time += time.time() - start
        results['tldextract'] = total_time / iterations
    
    # Benchmark publicsuffix2 if available
    if has_publicsuffix2:
        total_time = 0
        for _ in range(iterations):
            start = time.time()
            for url in urls:
                try:
                    publicsuffix2.get_sld(url)
                except:
                    pass
            total_time += time.time() - start
        results['publicsuffix2'] = total_time / iterations
    
    return results

# Example usage
# urls = ["https://example.com", "https://mail.google.com", ...] * 1000
# results = benchmark(urls)
# print(results)
```

## Conclusion

This advanced guide covers the more complex aspects of using liburlparser in Python. By leveraging these techniques, you can build high-performance applications that efficiently process and analyze URLs and domains.

For more information, check out the [API Reference](../api/host.md) or the [Examples](../examples.md) page.