/*
 * liburlparser C++ benchmark.
 *
 * Peers benchmarked here:
 *   - liburlparser  this project (github.com/mohammadraziei/liburlparser)
 *   - ada           github.com/ada-url/ada — WHATWG-compliant URL parser,
 *                   used in Node.js, Cloudflare Workers, ClickHouse, etc.
 *
 * ada has no Public Suffix List / domain-extraction feature (it's a pure
 * WHATWG URL parser), so "extract_from_host" only has one row - that's not
 * a contradiction, it's a real capability gap between the two libraries,
 * reported honestly rather than papered over. "parse_url" is the
 * apples-to-apples operation: both libraries just split a URL into its
 * components.
 */

#include "urlparser.h"
#include <ada.h>

#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef URLPARSER_BENCH_DOMAINS_FILE
#error "URLPARSER_BENCH_DOMAINS_FILE must be defined at compile time"
#endif
#ifndef URLPARSER_BENCH_URLS_FILE
#error "URLPARSER_BENCH_URLS_FILE must be defined at compile time"
#endif
#ifndef URLPARSER_BENCH_RESULTS_JSON
#error "URLPARSER_BENCH_RESULTS_JSON must be defined at compile time"
#endif
#ifndef URLPARSER_BENCH_REPEATS
#define URLPARSER_BENCH_REPEATS 20
#endif

namespace {

struct Result {
    std::string library, operation;
    double throughput_mb_s, ops_per_sec, success_rate, total_time_s;
};

std::vector<Result> g_results;

double now_seconds() {
    return static_cast<double>(std::clock()) / static_cast<double>(CLOCKS_PER_SEC);
}

std::vector<std::string> read_lines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

void record(const char* library, const char* operation,
            double bytes, long ops, double seconds, long attempted) {
    Result r;
    r.library = library;
    r.operation = operation;
    r.throughput_mb_s = ops ? bytes / seconds / 1e6 : 0.0;
    r.ops_per_sec = static_cast<double>(ops) / seconds;
    r.success_rate = attempted ? static_cast<double>(ops) / static_cast<double>(attempted) : 0.0;
    r.total_time_s = seconds;
    g_results.push_back(r);

    std::printf("%-14s %-18s %9.2f MB/s %14.0f %8.0f%%  %8.4f s  (x%d reps)\n",
                library, operation, r.throughput_mb_s, r.ops_per_sec,
                r.success_rate * 100.0, seconds, URLPARSER_BENCH_REPEATS);
}

void write_results_json(size_t n_domains, size_t n_urls) {
    std::ofstream f(URLPARSER_BENCH_RESULTS_JSON);
    if (!f) {
        std::fprintf(stderr, "warning: could not write %s\n", URLPARSER_BENCH_RESULTS_JSON);
        return;
    }
    f << "{\n  \"language\": \"cpp\",\n";
    f << "  \"corpus\": {\"domains\": " << n_domains << ", \"urls\": " << n_urls << "},\n";
    f << "  \"results\": [\n";
    for (size_t i = 0; i < g_results.size(); i++) {
        const Result& r = g_results[i];
        char buf[512];
        std::snprintf(buf, sizeof(buf),
            "    {\"library\": \"%s\", \"operation\": \"%s\", "
            "\"throughput_mb_s\": %.4f, \"ops_per_sec\": %.2f, "
            "\"success_rate\": %.4f, \"total_time_s\": %.6f}%s\n",
            r.library.c_str(), r.operation.c_str(), r.throughput_mb_s,
            r.ops_per_sec, r.success_rate, r.total_time_s,
            (i + 1 < g_results.size()) ? "," : "");
        f << buf;
    }
    f << "  ]\n}\n";
    std::printf("\nResults written to %s\n", URLPARSER_BENCH_RESULTS_JSON);
}

}  // namespace

int main() {
    auto domains = read_lines(URLPARSER_BENCH_DOMAINS_FILE);
    auto urls = read_lines(URLPARSER_BENCH_URLS_FILE);
    if (domains.empty() || urls.empty()) {
        std::fprintf(stderr, "Corpus is empty - nothing to benchmark.\n");
        return 1;
    }

    std::size_t domains_bytes = 0;
    for (const auto& d : domains) domains_bytes += d.size();
    std::size_t urls_bytes = 0;
    for (const auto& u : urls) urls_bytes += u.size();

    std::printf("liburlparser Benchmarks — C++\n");
    std::printf("Corpus: %zu domains, %zu URLs\n\n", domains.size(), urls.size());
    std::printf("%-14s %-18s %12s %14s %9s %10s %12s\n",
                "Library", "Operation", "Throughput", "Ops/sec", "Success", "", "Total time");

    // Untimed warm-up: liburlparser's Public Suffix List is loaded lazily on
    // first use (a function-local static, see psl::instance() in
    // urlparser.cpp) - trigger that load here so it doesn't pollute the
    // first timed measurement below.
    { volatile bool warm = urlparser::url::is_psl_loaded(); (void)warm;
      urlparser::hostname warmup(domains.front()); (void)warmup; }

    // ── extract_from_host: liburlparser only ───────────────────────────
    // ada has no PSL / domain-extraction feature - there is no
    // "ada::extract_domain" to compare against, so this operation reports
    // a single row. That's a real capability difference, not an omission.
    {
        long ops = 0; double bytes = 0;
        double t0 = now_seconds();
        for (int rep = 0; rep < URLPARSER_BENCH_REPEATS; rep++) {
            for (const auto& d : domains) {
                try {
                    urlparser::hostname h(d);
                    auto s = h.suffix();
                    (void)s.size();
                    ops++;
                    bytes += static_cast<double>(d.size());
                } catch (const std::exception&) {
                }
            }
        }
        record("liburlparser", "extract_from_host", bytes, ops, now_seconds() - t0,
               static_cast<long>(domains.size()) * URLPARSER_BENCH_REPEATS);
    }

    // ── parse_url: liburlparser vs ada ──────────────────────────────────
    {
        long ops = 0; double bytes = 0;
        double t0 = now_seconds();
        for (int rep = 0; rep < URLPARSER_BENCH_REPEATS; rep++) {
            for (const auto& u : urls) {
                try {
                    urlparser::url parsed(u);
                    auto p = parsed.protocol();
                    auto h = parsed.host_text();
                    auto path = parsed.abspath();
                    auto q = parsed.query();
                    auto frag = parsed.fragment();
                    (void)p.size(); (void)h.size(); (void)path.size();
                    (void)q.size(); (void)frag.size();
                    ops++;
                    bytes += static_cast<double>(u.size());
                } catch (const std::exception&) {
                }
            }
        }
        record("liburlparser", "parse_url", bytes, ops, now_seconds() - t0,
               static_cast<long>(urls.size()) * URLPARSER_BENCH_REPEATS);
    }
    {
        long ops = 0; double bytes = 0;
        double t0 = now_seconds();
        for (int rep = 0; rep < URLPARSER_BENCH_REPEATS; rep++) {
            for (const auto& u : urls) {
                auto result = ada::parse<ada::url_aggregator>(u);
                if (!result) continue;
                auto p = result->get_protocol();
                auto h = result->get_host();
                auto path = result->get_pathname();
                auto q = result->get_search();
                auto frag = result->get_hash();
                (void)p.size(); (void)h.size(); (void)path.size();
                (void)q.size(); (void)frag.size();
                ops++;
                bytes += static_cast<double>(u.size());
            }
        }
        record("ada", "parse_url", bytes, ops, now_seconds() - t0,
               static_cast<long>(urls.size()) * URLPARSER_BENCH_REPEATS);
    }

    write_results_json(domains.size(), urls.size());
    return 0;
}
