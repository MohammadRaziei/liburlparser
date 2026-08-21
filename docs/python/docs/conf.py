# Configuration file for Sphinx documentation (Python API).

from pathlib import Path
import re
import sys

DOCS_DIR     = Path(__file__).resolve().parent
PROJECT_ROOT = DOCS_DIR.parent.parent.parent
IMAGES_DIR   = PROJECT_ROOT / 'docs' / 'images'

# ── Version ─────────────────────────────────────────────────────────────────
# liburlparser keeps its version in pyproject.toml (not a C header).
PYPROJECT_TOML = PROJECT_ROOT / 'pyproject.toml'
version = '0.0.0'

if PYPROJECT_TOML.exists():
    text  = PYPROJECT_TOML.read_text()
    match = re.search(r'(?m)^version\s*=\s*"(\d+\.\d+\.\d+)"', text)
    if match:
        version = match.group(1)

# ── Import liburlparser from the installed build ─────────────────────────────
sys.path.insert(0, '.')

import liburlparser  # noqa: E402

# ── Project info ──────────────────────────────────────────────────────────────
project   = 'liburlparser Python'
copyright = '2023-present, Mohammad Raziei'
author    = 'Mohammad Raziei'
release   = version

# ── Extensions ────────────────────────────────────────────────────────────────
extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.napoleon',
    'sphinx.ext.viewcode',
    'sphinx.ext.autosummary',
    'myst_parser',
]

# Render the existing hand-written .md guide pages directly (no rst rewrite
# needed) alongside the autodoc-generated API reference.
source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}
myst_enable_extensions = ['colon_fence', 'deflist']

autodoc_member_order   = 'groupwise'
autodoc_typehints      = 'description'
autosummary_generate   = True

# ── Hide internal liburlparser.core module — rewrite to liburlparser ────────
add_module_names       = False
modindex_common_prefix = ['liburlparser.']


def _fix_core_module(app, what, name, obj, options, lines):
    """Rewrite liburlparser.core / liburlparser._core → liburlparser in docstrings."""
    for i, line in enumerate(lines):
        lines[i] = (
            line.replace('liburlparser.core.', 'liburlparser.')
                .replace('liburlparser._core.', 'liburlparser.')
        )


def _fix_core_module_sig(app, what, name, obj, options, signature, return_annotation):
    if signature:
        signature = signature.replace('liburlparser.core.', 'liburlparser.').replace('liburlparser._core.', 'liburlparser.')
    if return_annotation:
        return_annotation = return_annotation.replace('liburlparser.core.', 'liburlparser.').replace('liburlparser._core.', 'liburlparser.')
    return signature, return_annotation


def setup(app):
    app.connect('autodoc-process-docstring', _fix_core_module)
    app.connect('autodoc-process-signature', _fix_core_module_sig)


napoleon_google_docstring = True
napoleon_numpy_docstring  = True

# ── HTML output ───────────────────────────────────────────────────────────────
language         = 'en'
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']
html_theme       = 'furo'
pygments_style   = 'monokai'

_logo = IMAGES_DIR / 'logo' / 'liburlparser-logo-2.svg'
if _logo.exists():
    html_logo = str(_logo)

html_last_updated_fmt = '%Y-%m-%d %H:%M'
