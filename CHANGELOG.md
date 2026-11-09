# Changelog

All notable changes to liburlparser are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project follows
[Semantic Versioning](https://semver.org/). The version of the project is the one in `include/urlparser.h`
(`URLPARSER_VERSION_MAJOR/MINOR/PATCH`); `pyproject.toml` and CMake read it from there.

The entries up to 2.1.0 were written from the git history and the release tags. Since the commit messages of the 1.x
releases are short, those entries are brief.

## [2.2.0] - 2026-11-09

### Changed
- **The CMake package namespace is now `liburlparser::`, like the package.** The public target is
  `liburlparser::urlparser`, the same name from `add_subdirectory()`, `FetchContent` and
  `find_package(liburlparser CONFIG)`. A CMake package should export its targets under its own name, and the
  package is `liburlparser` (`liburlparserConfig.cmake`, `python -m liburlparser --cmake-dir`). **Breaking:** the
  2.1.0 name `urlparser::urlparser` is gone and there is no alias for it. Change `target_link_libraries(... urlparser::urlparser)` to
  `liburlparser::urlparser`. The C++ namespace (`urlparser::url`, `urlparser::host`) is unchanged.
- The `example` program is built only when liburlparser is the top-level project, like the tests and the docs, so it is
  not compiled for nothing when the library is a dependency.

### Fixed
- **Using liburlparser from another scikit-build-core project failed.** `SKBUILD` is defined for the whole build, so a
  project that pulled liburlparser in with `add_subdirectory()` or `FetchContent` made it believe it was itself the wheel
  being built: it tried to build its own nanobind module (and failed when nanobind was not installed) or stopped
  with `RETURN()` before the example, tests and docs. The check is now "`SKBUILD` is defined **and** liburlparser is the
  top-level project". The 2.0.0 release had a guard for this (`c140ea4`); the CMake refactor of 2.1.0 (`ac6ac03`)
  dropped it.

## [2.1.0] - 2026-10-03

### Added
- **`find_package(liburlparser CONFIG)`.** The core is a static library that is installed with its headers and a CMake
  package (`liburlparserConfig.cmake`, targets, version file, and `liburlparserPaths.cmake` with the absolute library
  and include paths). The Python wheel carries the same tree (`include/`, `lib/`, `cmake/`), and
  `python -m liburlparser --cmake-dir` / `--include-dir` / `--lib-dir` print the paths, the way `python -m nanobind --cmake_dir` does.
  `examples/find_liburlparser_via_python` shows it.
- **IDNA normalization and percent codec APIs.**
- **scp-like git URLs** (`user@host:path/repo.git`) are parsed.
- **`url::normalized()`** returns a canonical serialization of a URL.
- Benchmarks: `search_params` against ada, a reproducible comparison of the PSL structures, benchmark artifacts kept apart
  from the committed results, and an HTML report target.

### Changed
- **The CMake targets are unified.** The historical aliases `url::base` and `liburlparser::core` are replaced by the one
  public alias `urlparser::urlparser`, and `install(EXPORT)` uses the same name. (**Breaking** for anyone who linked the old aliases;
  renamed again in 2.2.0.)
- The standalone `idna.cpp` and `percent_codec.cpp` are merged into `urlparser.cpp`, and stale files left from the
  header/source consolidation were removed.
- The bundled `public_suffix_list.dat` is the 2026-09-08 version.
- CI: cibuildwheel is updated.

### Performance
- AVX2 scanning for the authority of a URL.
- PSL suffix matching tracks the maximum rule depth, and the truncation scan in `suffix_of` and the TLD suffix are bounded
  by the maximum length (an O(1) check instead of an O(n) scan).
- A fast path with interned keys when a `Hostname` is extracted to a Python dict.

## [2.0.0] - 2026-09-11

### Added
- **Public Suffix List support.** A `psl` class (a singleton that tracks the version of its data) matches suffixes; the
  list is embedded into the library when CMake configures, and a configure step refreshes the tracked
  `public_suffix_list.dat` in place when it can download a newer one.
- **`host`** parsing and classification, with constructor overloads and `string&&` overloads of `host::from_url` and
  `url::extract_host`. Cross-platform IP address parsing replaces the platform-specific code.
- **Version macros** `URLPARSER_VERSION_*` and a version class (the prefix was renamed from `LIBURLPARSER_`); CMake reads the version from the
  header.
- SIMD scanning in `find_first_of_3` (AVX2, SSE2, NEON).
- Python: a static `Url.extract_dict`, built from reusable `to_dict` helpers.
- Tooling: coverage reports for C++ and Python, leak checks with pytest-memray, benchmark and profiling targets, standalone
  benchmarks (also against ada), Sphinx documentation for the Python API, a CLI reference, and architecture notes.

### Changed
- **liburlparser is a static library**, built with position-independent code, and the default build type is Release.
- nanobind ABI handling for Python 3.12 and later.
- The googletest and doxygen-awesome-css submodules are removed, and the `online` extra is gone (the PSL update uses `urllib`).
- Tests and docs are built only when liburlparser is the top-level project, and a vendored copy does not build its Python
  bindings.
- The implementation of the suffix table is hidden behind a forward declaration.

### Fixed
- Windows compatibility (pull request #4, by CKCat).
- Regression tests for earlier URL parsing bugs.

### Performance
- Extra capacity for URL fields, a faster `host` constructor, and the suffix position is computed by arithmetic.

## [1.6.1] - 2025-10-24

### Changed
- The CLI logic is refactored and covered by tests, and `flatten_dict` moved to the utils module.
- The release workflow and the wheel builds are improved (`test_cli_subprocess` is excluded in CI).

## [1.6.0] - 2025-05-04

### Added
- `--parts` option of the command line, and the top-level domain (TLD) of a host.
- The Public Suffix List file.
- Doxygen documentation (main page with logo and architecture diagram, doxygen-awesome theme) and the Python documentation,
  published to GitHub Pages.

## [1.5.0] - 2024-10-18

### Changed
- The project architecture changed, and the CI runs all tests (C++ and Python).
- Build requirements are managed with pip-tools; ruff fixes.

## [1.4.6] - 2024-02-11

### Changed
- macOS is built in CI, the build system changed, and the README got badges.

## [1.4.5] - 2023-11-28

### Changed
- The Doxygen project information is read from `pyproject.toml`, and the release workflow uploads the release assets.

## [1.4.4] - 2023-11-23

### Changed
- The C++ tests moved to googletest and were rewritten as parametrized tests, like the Python ones; C++ and Python
  tests are separate. The wheel files are created.

## [1.4.3] - 2023-11-19

### Fixed
- A file lock lets the tests run in parallel.

## [1.4.2] - 2023-11-13

### Changed
- `shared_ptr` replaces `unique_ptr`, since every method is const.

### Fixed
- The `fulldomain` problem, and the host name was removed again.

## [1.4.1] - 2023-11-11

### Added
- C++ tests with gtest and gmock.
- A fast `hostName` that does not parse the host string (with a known problem with `ignore_www`).

### Changed
- The PSL is held as data instead of a pointer (fixes the pointer-to-data conversion), and `requests` became an optional
  dependency (the `online` extra).

## [1.4.0] - 2023-11-08

### Changed
- `urlparser.cpp` is split into a host part and a url part, a url holds its host through a pointer, and the lazy
  calculation uses a smart pointer.

### Fixed
- Problems with the operators; the move constructor and assignment were removed.

## [1.3.0] - 2023-11-05

### Fixed
- `fulldomain` in the substring step; accessibility of the class; test and linter issues. An automatic correction was
  tried and removed.

## [1.2.1] - 2023-10-10

### Fixed
- The `fulldomain` issue.

## [1.2.0] - 2023-10-09

### Changed
- The `fulldomain` attribute was updated.

## [1.1.0] - 2023-07-23

### Added
- The `ignore_www` option for subdomains.

## [1.0.2] - 2023-07-22

### Added
- A Ruff lint check.

## [1.0.1] - 2023-07-21

### Fixed
- `pyproject.toml` and the wheels workflow.

## [1.0.0] - 2023-07-21

First tagged release. The history before it is mostly the setup of the wheel builds with cibuildwheel (PyPy excluded).

[2.2.0]: https://github.com/mohammadraziei/liburlparser/compare/v2.1.0...v2.2.0
[2.1.0]: https://github.com/mohammadraziei/liburlparser/compare/v2.0.0...v2.1.0
[2.0.0]: https://github.com/mohammadraziei/liburlparser/compare/v1.6.1...v2.0.0
[1.6.1]: https://github.com/mohammadraziei/liburlparser/compare/v1.6.0...v1.6.1
[1.6.0]: https://github.com/mohammadraziei/liburlparser/compare/v1.5.0...v1.6.0
[1.5.0]: https://github.com/mohammadraziei/liburlparser/compare/v1.4.6...v1.5.0
[1.4.6]: https://github.com/mohammadraziei/liburlparser/compare/v1.4.5...v1.4.6
[1.4.5]: https://github.com/mohammadraziei/liburlparser/compare/v1.4.4...v1.4.5
[1.4.4]: https://github.com/mohammadraziei/liburlparser/compare/v1.4.3...v1.4.4
[1.4.3]: https://github.com/mohammadraziei/liburlparser/compare/v1.4.2...v1.4.3
[1.4.2]: https://github.com/mohammadraziei/liburlparser/compare/v1.4.1...v1.4.2
[1.4.1]: https://github.com/mohammadraziei/liburlparser/compare/v1.4.0...v1.4.1
[1.4.0]: https://github.com/mohammadraziei/liburlparser/compare/v1.3.0...v1.4.0
[1.3.0]: https://github.com/mohammadraziei/liburlparser/compare/v1.2.1...v1.3.0
[1.2.1]: https://github.com/mohammadraziei/liburlparser/compare/v1.2.0...v1.2.1
[1.2.0]: https://github.com/mohammadraziei/liburlparser/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/mohammadraziei/liburlparser/compare/v1.0.2...v1.1.0
[1.0.2]: https://github.com/mohammadraziei/liburlparser/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/mohammadraziei/liburlparser/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/mohammadraziei/liburlparser/releases/tag/v1.0.0
