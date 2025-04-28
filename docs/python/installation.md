# Installation

liburlparser requires Python 3.8 or higher.

## Using pip (Recommended)

Install from PyPI:

```bash
pip install liburlparser
```

If you want to use the `psl.update` feature to update the public suffix list, install the `online` version:

```bash
pip install "liburlparser[online]"
```

## Install from GitHub

```bash
pip install git+https://github.com/mohammadraziei/liburlparser
```

## Manual Installation

```bash
git clone https://github.com/mohammadraziei/liburlparser
pip install ./liburlparser
```

## Verifying Installation

After installation, you can verify that liburlparser is working correctly:

```python
import liburlparser
print(liburlparser.__version__)

# Try parsing a URL
from liburlparser import Url
url = Url("https://example.com")
print(url.domain)  # Should output: example
```

## Dependencies

liburlparser has minimal dependencies:

- Python 3.8 or higher

The `online` version additionally requires:
- requests (for updating the Public Suffix List)

## Platform Support

liburlparser works on:

- Windows
- macOS
- Linux

## Troubleshooting

If you encounter any issues during installation:

1. Make sure you have Python 3.8 or higher:
   ```bash
   python --version
   ```

2. Ensure you have the latest pip:
   ```bash
   pip install --upgrade pip
   ```

3. If you're on Windows and encounter build issues, you may need to install Visual C++ build tools:
   ```bash
   pip install --upgrade setuptools wheel
   ```

4. If you're on Linux and encounter build issues, you may need to install development tools:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install python3-dev build-essential
   
   # Fedora/RHEL/CentOS
   sudo dnf install python3-devel gcc
   ```

If you continue to experience issues, please [open an issue on GitHub](https://github.com/mohammadraziei/liburlparser/issues).