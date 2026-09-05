# Examples

## Parse a full URL

```bash
python -m liburlparser --url "https://www.example.com/about?x=1"
```

Prints the full parsed URL as JSON.

## Classify a bare host

`--host` works for domains, IPv4, and IPv6 alike — the CLI picks the
right type automatically:

```bash
python -m liburlparser --host "example.com"
python -m liburlparser --host "192.168.1.1"
python -m liburlparser --host "::1"
```

## Extract specific fields

```bash
python -m liburlparser --url "https://www.example.com/about" --parts domain suffix
```

```
example com
```

## Scripting: get just the domain

```bash
domain=$(python -m liburlparser --url "$1" --parts domain)
echo "domain: $domain"
```

## Version and docstring

```bash
python -m liburlparser --version
python -m liburlparser --doc
```
