"""
generate_report.py — combine benchmarks/*.json (one per language) into
one standalone HTML report: Chart.js and all the raw JSON data are
embedded directly in the file (via the vendored chart.umd.min.js and
inline <script> tags), so the output needs nothing else to view or to
keep - no server, no network, no sibling files. That's also why this is
the one artifact meant to live in benchmarks/results/: everything
upstream of it (corpus, venv, per-language JSON) is disposable build
output (see benchmarks/CMakeLists.txt for that split).

Report structure: one *section* per operation (extract_from_host,
parse_url, idna_normalize, ...), and within each, one *subsection* per
language (C++ first, then Python) - each subsection's chart/table only
ever shows the libraries actually benchmarked in that language, since a
C++-only competitor (ada) and a Python-only one (tldextract) were never
run against each other and showing them on one merged chart implied a
comparison that was never made.

Usage:
    generate_report.py <scratch_dir> <report_html_out> --chartjs-path <path>

<scratch_dir> is expected to contain cpp.json and python.json, each
shaped like {"language": ..., "corpus": {...}, "results": [{"library",
"operation", "throughput_mb_s", "ops_per_sec", "success_rate",
"total_time_s"}, ...]} - see benchmarks/cpp/bench.cpp's
write_results_json() and benchmarks/python/bench.py's json.dump() call,
the two (and only two) producers of this shape.
"""
import argparse
import datetime
import json
import os

from jinja2 import Environment, FileSystemLoader

HERE = os.path.dirname(os.path.abspath(__file__))

# Colors inherited from the project's own docs site theme (dark-mode
# variables in docs/urlparser-docs.css's `:root[data-theme="dark"]` block)
# rather than any generic/borrowed palette - this report is a liburlparser
# artifact, so it looks like one.
COLORS = {
    "bg": "#0f0f12",
    "fg": "#e8e8f0",
    "fg_dim": "#9a9aa8",
    "border": "#2a2a35",
    "accent": "#679dcb",
    "accent_hover": "#8fb8db",
    "gold": "#ffd340",
    "card": "#1a1a1f",
    "card_border": "#2a2a35",
    "code_bg": "#13131a",
    "code_fg": "#e0e0f0",
}

# liburlparser (the subject) always gets the project's own accent color;
# everyone else gets a distinct hue from a small fixed, colorblind-friendlier
# palette, assigned in first-seen order so the same competitor keeps the
# same color across every chart in the report.
COMPETITOR_PALETTE = [
    "#ffd340",  # gold (the project's own secondary accent)
    "#e08a7a",  # coral
    "#9d8fd9",  # violet
    "#7fd1ae",  # sage green
    "#e0a458",  # amber
    "#7ab8cc",  # teal
    "#c98fc9",  # orchid
]

LANGUAGE_LABELS = {"cpp": "C++", "python": "Python"}
LANGUAGE_ORDER = ["cpp", "python"]  # C++ first in every section, always

# Human-friendly operation names, in the display order they should appear.
OPERATION_LABELS = [
    ("extract_from_host", "Extract domain/suffix from a host"),
    ("extract_from_url", "Extract domain/suffix from a full URL"),
    ("parse_url", "Parse a full URL"),
    ("idna_normalize", "IDNA: Unicode host → ASCII/Punycode"),
    ("percent_decode", "Percent-decode (\"unquote\")"),
    ("percent_encode", "Percent-encode (\"quote\")"),
    ("resolve", "Resolve a URL reference (RFC 3986 §5)"),
    ("parse_git_url", "Parse a git/scp-like remote address"),
    ("normalize", "Normalize a URL (default port, IDNA, dot-segments)"),
]


def _load(path):
    if path and os.path.exists(path):
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    return None


def _fmt_num(n, digits=2):
    if n is None:
        return "—"
    return f"{n:,.{digits}f}"


def build_sections(all_results):
    """Group every {language, results:[...]} payload's rows by operation,
    then by language within that - a C++-only competitor (ada) and a
    Python-only one (tldextract) were never actually run against each
    other, so they never share a chart; only liburlparser bridges the two
    subsections of a section, by definition (it's the one library run in
    both)."""
    by_operation = {}
    competitor_color = {}
    next_color_idx = 0

    for payload in all_results:
        if not payload:
            continue
        language = payload.get("language", "?")
        for row in payload.get("results", []):
            op = row["operation"]
            by_operation.setdefault(op, {}).setdefault(language, []).append(row)
            lib = row["library"]
            if lib != "liburlparser" and lib not in competitor_color:
                competitor_color[lib] = COMPETITOR_PALETTE[next_color_idx % len(COMPETITOR_PALETTE)]
                next_color_idx += 1

    def color_for(library):
        return COLORS["accent"] if library == "liburlparser" else competitor_color.get(library, COLORS["gold"])

    sections = []
    labels = dict(OPERATION_LABELS)
    ordered_ops = [op for op, _ in OPERATION_LABELS] + \
        [op for op in by_operation if op not in labels]

    for op in ordered_ops:
        if op not in by_operation:
            continue
        by_language = by_operation[op]
        subsections = []
        for lang in LANGUAGE_ORDER:
            if lang not in by_language:
                continue
            rows = sorted(by_language[lang],
                          key=lambda r: (r["library"] != "liburlparser",
                                         -(r["throughput_mb_s"] or 0)))
            subsections.append({
                "language": lang,
                "language_label": LANGUAGE_LABELS.get(lang, lang),
                "rows": rows,
                "chart_id": f"chart-{op}-{lang}",
                "chart_labels": [r["library"] for r in rows],
                "chart_throughput": [round(r["throughput_mb_s"] or 0, 3) for r in rows],
                "chart_ops": [round(r["ops_per_sec"] or 0, 1) for r in rows],
                "chart_colors": [color_for(r["library"]) for r in rows],
            })
        for lang, rows in by_language.items():
            if lang in LANGUAGE_ORDER:
                continue
            rows = sorted(rows, key=lambda r: (r["library"] != "liburlparser",
                                                -(r["throughput_mb_s"] or 0)))
            subsections.append({
                "language": lang, "language_label": lang, "rows": rows,
                "chart_id": f"chart-{op}-{lang}",
                "chart_labels": [r["library"] for r in rows],
                "chart_throughput": [round(r["throughput_mb_s"] or 0, 3) for r in rows],
                "chart_ops": [round(r["ops_per_sec"] or 0, 1) for r in rows],
                "chart_colors": [color_for(r["library"]) for r in rows],
            })
        if subsections:
            sections.append({"id": op, "title": labels.get(op, op), "subsections": subsections})

    return sections


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("scratch_dir", help="Directory containing cpp.json / python.json")
    parser.add_argument("report_html_out")
    parser.add_argument("--chartjs-path", required=True,
                         help="Path to the vendored chart.umd.min.js to embed")
    args = parser.parse_args()

    cpp_results = _load(os.path.join(args.scratch_dir, "cpp.json"))
    python_results = _load(os.path.join(args.scratch_dir, "python.json"))
    sections = build_sections([cpp_results, python_results])

    with open(args.chartjs_path, "r", encoding="utf-8") as f:
        chartjs_source = f.read().replace("</script", "<\\/script")

    corpus = (cpp_results or python_results or {}).get("corpus", {})

    env = Environment(loader=FileSystemLoader(HERE), autoescape=True)
    env.filters["fmt_num"] = _fmt_num
    template = env.get_template("template.html.jinja2")

    html = template.render(
        generated_at=datetime.datetime.now(datetime.timezone.utc)
            .strftime("%Y-%m-%d %H:%M UTC"),
        corpus=corpus,
        sections=sections,
        colors=COLORS,
        chartjs_source=chartjs_source,
        sections_json=json.dumps(sections).replace("</script", "<\\/script"),
        cpp_present=cpp_results is not None,
        python_present=python_results is not None,
    )

    os.makedirs(os.path.dirname(os.path.abspath(args.report_html_out)), exist_ok=True)
    with open(args.report_html_out, "w", encoding="utf-8") as f:
        f.write(html)

    print(f"Report written to {args.report_html_out}")


if __name__ == "__main__":
    main()
