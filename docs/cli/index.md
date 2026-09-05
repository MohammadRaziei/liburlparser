# CLI Reference

liburlparser ships a small command-line tool through its Python module —
`python -m liburlparser` — for parsing a URL or a bare host straight from
the terminal, without writing any Python.

## Synopsis

```bash
python -m liburlparser --url <URL> [--parts PART [PART ...]]
python -m liburlparser --host <HOST> [--parts PART [PART ...]]
python -m liburlparser --version
python -m liburlparser --doc
```

Exactly one of `--url` / `--host` may be given per invocation.

## Quick start

```bash
# Install
pip install liburlparser

# Parse a full URL, print it as JSON
python -m liburlparser --url "https://www.example.com/about?x=1"

# Classify a bare host (domain, IPv4, or IPv6 - picked automatically)
python -m liburlparser --host "192.168.1.1"

# Pull out just the parts you want
python -m liburlparser --url "https://www.example.com/about" --parts domain suffix
```
