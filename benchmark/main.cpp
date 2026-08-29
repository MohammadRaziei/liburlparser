// Standalone benchmark for liburlparser.
//
// Build:  cmake --build build --target urlparser_benchmark
// Run:    ./build/benchmark/urlparser_benchmark [iterations]
//
// This exists to make performance regressions/regains visible and
// measurable, rather than argued about. It generates a synthetic but
// realistic corpus of URLs (mixing bare domains, multi-level PSL suffixes
// like "co.uk"/"com.au", subdomains, and "www." prefixes) and times a few
// distinct usage patterns:
//
//   1. url(...)                         - just parsing the URL structure
//   2. host::from_url(...)               - constructing a host (should be cheap: no PSL work)
//   3. host::from_url(...).full_domain()  - the common case (ignore_www=false):
//                                          should hit the PSL-free fast path
//   4. host::from_url(...).domain()      - forces the PSL lookup (lazy, cached)
//
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "urlparser.h"

namespace {

std::vector<std::string> makeCorpus(size_t count) {
    static const char* templates[] = {
        "https://www.example.com/path/to/page?query=1",
        "http://sub.example.co.uk/",
        "https://api.github.io/v1/resource",
        "https://blog.subdomain.example.com.au/post/123",
        "http://plain.org",
        "https://www.deep.nested.subdomain.example.net/a/b/c",
        "https://shop.example.io/cart?id=42#top",
        "http://mail.google.com/mail/u/0/",
        "https://www.bbc.co.uk/news/uk-12345678",
        "https://accounts.example.gov.uk/login",
    };
    constexpr size_t n_templates = sizeof(templates) / sizeof(templates[0]);

    std::vector<std::string> corpus;
    corpus.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        corpus.push_back(templates[i % n_templates]);
    }
    return corpus;
}

template <typename Fn>
double timeIt(const char* label, size_t iterations, Fn&& fn) {
    const auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        fn(i);
    }
    const auto end = std::chrono::steady_clock::now();

    const double total_ns =
        std::chrono::duration<double, std::nano>(end - start).count();
    const double ns_per_op = total_ns / static_cast<double>(iterations);
    const double ops_per_sec = 1e9 / ns_per_op;

    std::printf("%-45s %10.1f ns/op   %12.0f ops/sec\n", label, ns_per_op, ops_per_sec);
    return ns_per_op;
}

}  // namespace

int main(int argc, char** argv) {
    size_t iterations = 200000;
    if (argc > 1) {
        iterations = static_cast<size_t>(std::stoul(argv[1]));
    }

    std::printf("liburlparser benchmark - %zu iterations\n", iterations);
    std::printf("PSL loaded: %s\n\n", urlparser::url::is_psl_loaded() ? "yes" : "no");

    const std::vector<std::string> corpus = makeCorpus(iterations);
    volatile size_t sink = 0;  // prevents the optimizer from discarding results

    timeIt("url(url)", iterations, [&](size_t i) {
        urlparser::url u(corpus[i]);
        sink += u.host().full_domain().size();
    });

    timeIt("host::from_url(url)  [construct only]", iterations, [&](size_t i) {
        urlparser::host h = urlparser::host::from_url(corpus[i]);
        sink += reinterpret_cast<size_t>(&h) & 1;  // touch h, avoid dead-code elim
    });

    timeIt("host::from_url(url).full_domain()  [PSL-free path]", iterations, [&](size_t i) {
        urlparser::host h = urlparser::host::from_url(corpus[i]);
        sink += h.full_domain().size();
    });

    timeIt("host::from_url(url).domain()  [forces PSL lookup]", iterations, [&](size_t i) {
        urlparser::host h = urlparser::host::from_url(corpus[i]);
        sink += h.domain().size();
    });

    timeIt("host::from_url(url).domain()+.suffix()+.subdomain()", iterations, [&](size_t i) {
        urlparser::host h = urlparser::host::from_url(corpus[i]);
        sink += h.domain().size() + h.suffix().size() + h.subdomain().size();
    });

    std::printf("\n(sink=%zu, ignore this - just prevents dead-code elimination)\n", sink);
    return 0;
}
