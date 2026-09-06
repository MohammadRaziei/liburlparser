# CLI Reference

liburlparser ships a small command-line tool — `liburlparser` — for parsing
a URL or a bare host straight from the terminal, without writing any Python.
(`python -m liburlparser` works identically, if you prefer invoking it that way.)

## Synopsis

```bash
liburlparser --url <URL> [--parts PART [PART ...]]
liburlparser --host <HOST> [--parts PART [PART ...]]
liburlparser --version
liburlparser --doc
```

Exactly one of `--url` / `--host` may be given per invocation.

## Quick start

```bash
# Install
pip install liburlparser

# Parse a full URL, print it as JSON
liburlparser --url "https://www.example.com/about?x=1"

# Classify a bare host (domain, IPv4, or IPv6 - picked automatically)
liburlparser --host "192.168.1.1"

# Pull out just the parts you want
liburlparser --url "https://www.example.com/about" --parts domain suffix
```
