# PSL API Reference

The Public Suffix List (PSL) functionality in liburlparser is accessed through the global `psl` object.

## Overview

The Public Suffix List is a list of all known public suffixes, such as ".com", ".co.uk", etc. liburlparser uses this list to accurately parse domain names and determine the correct domain and suffix parts.

## Global Object

```python
from liburlparser import psl
```

The `psl` object is a global instance of the `Psl` class that is automatically created when you import liburlparser.

## Properties

### url

The URL of the Public Suffix List.

```python
from liburlparser import psl
print(psl.url)  # https://publicsuffix.org/list/public_suffix_list.dat
```

### filename

The filename of the Public Suffix List.

```python
from liburlparser import psl
print(psl.filename)  # public_suffix_list.dat
```

## Methods

### is_loaded

Check if the Public Suffix List is loaded.

```python
from liburlparser import psl
is_loaded = psl.is_loaded()
print(f"PSL loaded: {is_loaded}")  # PSL loaded: True
```

### load_from_path

Load the Public Suffix List from a file.

```python
from liburlparser import psl
psl.load_from_path("/path/to/custom/public_suffix_list.dat")
```

### load_from_string

Load the Public Suffix List from a string.

```python
from liburlparser import psl
with open("/path/to/custom/public_suffix_list.dat", "r") as f:
    psl_content = f.read()
    psl.load_from_string(psl_content)
```

## Example Usage

```python
from liburlparser import psl

# Check if PSL is loaded
if psl.is_loaded():
    print(f"PSL is loaded from {psl.filename}")
else:
    print("PSL is not loaded, loading from custom path...")
    psl.load_from_path("/path/to/custom/public_suffix_list.dat")
    
# Print PSL information
print(f"PSL URL: {psl.url}")
print(f"PSL Filename: {psl.filename}")
```

## Updating the PSL (Online Version Only)

If you installed the online version (`pip install "liburlparser[online]"`), you can update the PSL:

```python
# This feature is only available if you installed with:
# pip install "liburlparser[online]"
from liburlparser import psl_updater

# Update the PSL
psl_updater.update()
```