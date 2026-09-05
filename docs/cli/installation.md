# Installation

## Option 1 — pip (recommended)

The Python package ships a pre-built native extension — no compiler or
CMake needed.

```bash
pip install liburlparser
```

After install, the CLI is available as a Python module:

```bash
python -m liburlparser --version
```

## Option 2 — build with CMake

If you need a custom build configuration, or a platform without a
pre-built wheel:

```bash
git clone https://github.com/mohammadraziei/liburlparser.git
cd liburlparser
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_PYTHON=ON
cmake --build build -j$(nproc)
```

This builds the `_urlparser_py` extension under `build/liburlparser/`;
add that directory to `PYTHONPATH` (or `pip install .` from the repo
root) to use `python -m liburlparser` against your local build.

Requires CMake 3.19+, a C++17-compatible compiler, and Python 3.10+.
