# Options

| Option | Description |
|---|---|
| `--url <URL>` | Parse a full URL (e.g. `"https://google.com/about"`) |
| `--host <HOST>` | Parse just a host (e.g. `"google.com"`) — classified as a domain, IPv4, or IPv6 automatically |
| `--parts PART [PART ...]` | Print only the given fields, space-separated, instead of the full JSON |
| `-v`, `--version` | Print the installed liburlparser version and exit |
| `--doc` | Print the module's docstring and exit |
| `-h`, `--help` | Show the help message and exit |

`--url` and `--host` are mutually exclusive — exactly one may be given
per invocation, and running with neither (and no `--version`/`--doc`)
prints the help message.

## Output format

With no `--parts`, output is the full parsed object as JSON
(`obj.to_json()`). With `--parts`, only the requested field values are
printed, space-separated, on one line - handy for shell scripting:

```bash
python -m liburlparser --url "https://www.example.com/about" --parts domain suffix
```

If a requested part doesn't exist on the parsed object, the CLI exits
with status `1` and an error on stderr naming the invalid part.
