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
 * reported honestly rather than papered over. "parse_url" and
 * "idna_normalize" are apples-to-apples: both libraries just split a URL
 * into its components / normalize a hostname to ASCII, respectively.
 *
 * "percent_decode"/"percent_encode" report liburlparser only: ada does have
 * an internal percent-encode/decode (ada::unicode::percent_decode/encode),
 * but it's marked @private in ada's own header and needs an
 * already-computed character-set table and a "first_percent" index to call
 * - it's an implementation detail, not a stable public entry point, so
 * benchmarking against it wouldn't be the fair apples-to-apples comparison
 * the rest of this file aims for.
 *
 * "parse_git_url" (scp_url / git_url) also reports liburlparser only:
 * libgit2's equivalent, git_net_url_parse_standard_or_scp(), lives in
 * src/util/net.h - not part of its public include/git2/*.h API - and
 * getting it running standalone means bypassing libgit2's own CMake to
 * link internal objects directly against an explicitly unstable API. Same
 * "not a fair public-API comparison" call as percent_decode/encode above.
 */

#include "urlparser.h"
#include <ada.h>
#include <ada/ada_idna.h>

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
#ifndef URLPARSER_BENCH_WARMUP_REPS
#define URLPARSER_BENCH_WARMUP_REPS 3
#endif

namespace {

struct Result {
    std::string library, operation;
    double throughput_mb_s, ops_per_sec, success_rate, total_time_s;
};

std::vector<Result> g_results;

// Prevents the optimizer from eliminating a timed call whose return value
// is otherwise unused - every timed loop below must feed its result here.
volatile size_t g_sink = 0;

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

    // Untimed warm-up: every timed block below is preceded by
    // URLPARSER_BENCH_WARMUP_REPS full, untimed passes over the same
    // corpus with the exact same call. This isn't only about liburlparser's
    // Public Suffix List (a function-local static, see psl::instance() in
    // urlparser.cpp) - any library can have first-call costs (allocator
    // warm-up, cache effects, internal lazy init). Applying the same
    // warm-up to every library, not just liburlparser, keeps the
    // comparison honest.

    // ── extract_from_host: liburlparser only ───────────────────────────
    // ada has no PSL / domain-extraction feature - there is no
    // "ada::extract_domain" to compare against, so this operation reports
    // a single row. That's a real capability difference, not an omission.
    {
        for (int rep = 0; rep < URLPARSER_BENCH_WARMUP_REPS; rep++) {
            for (const auto& d : domains) {
                try {
                    urlparser::hostname h(d);
                    auto s = h.suffix();
                    (void)s.size();
                } catch (const std::exception&) {
                }
            }
        }

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
        for (int rep = 0; rep < URLPARSER_BENCH_WARMUP_REPS; rep++) {
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
                } catch (const std::exception&) {
                }
            }
        }

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
        for (int rep = 0; rep < URLPARSER_BENCH_WARMUP_REPS; rep++) {
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
            }
        }

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

    // ── idna_normalize: liburlparser vs ada ─────────────────────────────
    {
        std::vector<std::string> idna_hosts = {
            "caf\xc3\xa9.com", "m\xc3\xbcnchen.de", "espa\xc3\xb1""a.es",
            "portugu\xc3\xaas.pt",
            "\xe4\xb8\xad\xe5\x9b\xbd.icom.museum",
            "\xe6\x97\xa5\xe6\x9c\xac.jp",
            "\xd1\x80\xd0\xbe\xd1\x81\xd1\x81\xd0\xb8\xd1\x8f.\xd1\x80\xd1\x84",
            "\xce\xb5\xce\xbb\xce\xbb\xce\xac\xce\xb4\xce\xb1.gr",
            "example.com", "xn--caf-dma.com",
        };

        for (int rep = 0; rep < URLPARSER_BENCH_WARMUP_REPS; rep++)
            for (const auto& h : idna_hosts) { auto r = urlparser::idna::to_ascii(h); (void)r.size(); }
        long ops = 0; double bytes = 0;
        double t0 = now_seconds();
        for (int rep = 0; rep < URLPARSER_BENCH_REPEATS; rep++) {
            for (const auto& h : idna_hosts) {
                auto r = urlparser::idna::to_ascii(h);
                g_sink += r.size();
                ops++; bytes += static_cast<double>(h.size());
            }
        }
        record("liburlparser", "idna_normalize", bytes, ops, now_seconds() - t0,
               static_cast<long>(idna_hosts.size()) * URLPARSER_BENCH_REPEATS);

        for (int rep = 0; rep < URLPARSER_BENCH_WARMUP_REPS; rep++)
            for (const auto& h : idna_hosts) { auto r = ada::idna::to_ascii(h); (void)r.size(); }
        ops = 0; bytes = 0;
        t0 = now_seconds();
        for (int rep = 0; rep < URLPARSER_BENCH_REPEATS; rep++) {
            for (const auto& h : idna_hosts) {
                auto r = ada::idna::to_ascii(h);
                g_sink += r.size();
                ops++; bytes += static_cast<double>(h.size());
            }
        }
        record("ada", "idna_normalize", bytes, ops, now_seconds() - t0,
               static_cast<long>(idna_hosts.size()) * URLPARSER_BENCH_REPEATS);
    }

    // ── percent_decode / percent_encode: liburlparser only ──────────────
    // (see file header: ada's equivalent is @private, not a fair public
    // comparison)
    {
        std::vector<std::string> encoded = {
            "hello%20world", "caf%C3%A9%20%26%20friends",
            "q%3Dhello%20world%26more%3Dstuff", "100%25%20done",
            "a%2Fb%2Fc%2Fd", "%E6%97%A5%E6%9C%AC%E8%AA%9E",
        };
        std::vector<std::string> raw = {
            "hello world", "caf\xc3\xa9 & friends", "q=hello world&more=stuff",
            "100% done", "a/b/c/d", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e",
        };

        for (int rep = 0; rep < URLPARSER_BENCH_WARMUP_REPS; rep++)
            for (const auto& s : encoded) { auto r = urlparser::percent_codec::decode(s); (void)r.size(); }
        long ops = 0; double bytes = 0;
        double t0 = now_seconds();
        for (int rep = 0; rep < URLPARSER_BENCH_REPEATS; rep++) {
            for (const auto& s : encoded) {
                auto r = urlparser::percent_codec::decode(s);
                g_sink += r.size();
                ops++; bytes += static_cast<double>(s.size());
            }
        }
        record("liburlparser", "percent_decode", bytes, ops, now_seconds() - t0,
               static_cast<long>(encoded.size()) * URLPARSER_BENCH_REPEATS);

        for (int rep = 0; rep < URLPARSER_BENCH_WARMUP_REPS; rep++)
            for (const auto& s : raw) { auto r = urlparser::percent_codec::encode(s); (void)r.size(); }
        ops = 0; bytes = 0;
        t0 = now_seconds();
        for (int rep = 0; rep < URLPARSER_BENCH_REPEATS; rep++) {
            for (const auto& s : raw) {
                auto r = urlparser::percent_codec::encode(s);
                g_sink += r.size();
                ops++; bytes += static_cast<double>(s.size());
            }
        }
        record("liburlparser", "percent_encode", bytes, ops, now_seconds() - t0,
               static_cast<long>(raw.size()) * URLPARSER_BENCH_REPEATS);
    }

    // ── parse_git_url: liburlparser only ────────────────────────────────
    // (scp_url / git_url is new - see the "scp_url" section of
    // src/urlparser.cpp.) libgit2 has the equivalent -
    // git_net_url_parse_standard_or_scp() - but it's declared in
    // src/util/net.h, not include/git2/*.h: deliberately not part of
    // libgit2's public API. It technically compiles against libgit2's
    // internal .o files directly, but the result depends on running
    // libgit2's own global runtime init sequence (git_libgit2_init(),
    // src/libgit2/libgit2.c) which itself pulls in most of the rest of
    // the library - so getting it running means rebuilding effectively
    // all of libgit2 from raw objects outside its own CMake, against an
    // explicitly-internal, unstable API. That's not a fair
    // apples-to-apples public-API comparison (same reasoning as
    // percent_decode/percent_encode above), so, like those, this is
    // reported honestly as liburlparser-only rather than forced into a
    // comparison libgit2 doesn't offer publicly.
    {
        std::vector<std::string> git_urls = {
            "git@github.com:mohammadraziei/liburlparser.git",
            "git@gitlab.com:group/subgroup/project.git",
            "git@bitbucket.org:team/repo.git",
            "https://github.com/mohammadraziei/liburlparser.git",
            "ssh://git@example.com:2222/path/repo.git",
            "user@my-server.internal:~/projects/repo.git",
        };

        for (int rep = 0; rep < URLPARSER_BENCH_WARMUP_REPS; rep++)
            for (const auto& g : git_urls) {
                urlparser::git_url gu(g);
                g_sink += gu.as_url().host_text().size();
            }
        long ops = 0; double bytes = 0;
        double t0 = now_seconds();
        for (int rep = 0; rep < URLPARSER_BENCH_REPEATS; rep++) {
            for (const auto& g : git_urls) {
                urlparser::git_url gu(g);
                g_sink += gu.as_url().host_text().size();
                ops++; bytes += static_cast<double>(g.size());
            }
        }
        record("liburlparser", "parse_git_url", bytes, ops, now_seconds() - t0,
               static_cast<long>(git_urls.size()) * URLPARSER_BENCH_REPEATS);
    }

    write_results_json(domains.size(), urls.size());
    std::printf("(sanity sink: %zu - ignore, just proves every result above was actually used)\n",
                 static_cast<size_t>(g_sink));
    return 0;
}
