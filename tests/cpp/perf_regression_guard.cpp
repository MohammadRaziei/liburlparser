// Performance regression guard for CI.
//
// NOT a benchmark - a tripwire. Asserts current throughput stays above a
// conservative floor, set well below every number in OPTIMIZATION_NOTES.md
// (even accounting for slow/shared/virtualized CI hardware), so this only
// fails on a *gross* regression (a fast-path accidentally reverted, an
// allocation reintroduced into a hot loop, an O(n) check turning into
// O(n^2)) - not on ordinary CI noise. For real numbers with real
// comparisons against competitors, see benchmarks/, not this file.
//
// Deliberately has no corpus/network dependency (a small, representative,
// inline set of domains/paths instead of benchmarks/corpus/), so it can
// run in any CI job without the corpus-generation step.
//
// Always built at -O2/-O3 regardless of the parent build's CMAKE_BUILD_TYPE
// (see its CMakeLists.txt target_compile_options) - unlike the correctness
// test suite, which sometimes builds at -O0 for coverage instrumentation,
// where a timing assertion would be meaningless.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "urlparser.h"

namespace {

const std::vector<std::string>& sample_hosts() {
    static const std::vector<std::string> hosts = {
        "example.com",       "www.example.com",    "sub.example.co.uk",
        "blogspot.com",      "a.b.blogspot.com",    "github.io",
        "user.github.io",    "amazonaws.com",       "s3.us-east-1.amazonaws.com",
        "example.jp",        "city.kyoto.jp",       "example.com.au",
        "test.co",           "deeply.nested.sub.domain.example.org",
        "192.168.1.1",       "localhost",           "example.co.uk",
        "www.blogspot.co.uk","xn--bcher-kva.example","one.two.three.four.com",
    };
    return hosts;
}

const std::vector<std::string>& sample_paths() {
    // Mostly already-normalized (the common case abspath()'s fast path
    // targets) with a handful of real '.'/'..'/'//' cases mixed in, so
    // the guard exercises both branches, not just the fast one.
    static const std::vector<std::string> paths = {
        "/", "/a/b/c", "/articles/123/view", "/search?q=x",
        "/a/./b", "/a/../b", "/a//b//c", "/./a/b", "/a/b/..",
        "/very/long/path/that/is/already/clean/no/dots/here/at/all",
    };
    return paths;
}

template <class Fn>
double ops_per_sec(Fn&& fn, int reps) {
    const auto t0 = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r) fn();
    const auto t1 = std::chrono::steady_clock::now();
    return double(reps) / std::chrono::duration<double>(t1 - t0).count();
}

bool check(const char* name, double measured, double floor) {
    std::printf("%-32s %14.0f ops/s  (floor: %.0f)  %s\n", name, measured, floor,
                measured >= floor ? "OK" : "FAIL");
    return measured >= floor;
}

}  // namespace

int main() {
    bool all_ok = true;
    const auto& hosts = sample_hosts();
    const auto& paths = sample_paths();
    volatile size_t sink = 0;

    // hostname construction + PSL suffix lookup - guards against a
    // regression in psl::suffix_of()'s data structure or algorithm (see
    // OPTIMIZATION_NOTES.md section 1 and benchmarks/cpp/psl_structures/).
    double hostname_rate = ops_per_sec(
        [&] {
            for (auto& h : hosts) sink += urlparser::hostname(h).suffix().size();
        },
        2000);
    all_ok &= check("hostname construct+suffix_of", hostname_rate * hosts.size(), 1'000'000.0);

    // url::abspath() - guards against the fast-path/cache regressing back
    // to full dot-segment resolution on every call.
    //
    // Deliberately a generous absolute floor, not a tight one, and not a
    // ratio against a reference operation - both were tried. A same-run
    // ratio against std::string's copy constructor sounded more
    // hardware-independent on paper, but measured with 2x+ run-to-run
    // variance in practice even on unchanged code (small-string-
    // optimization and allocator behavior differ enough between "copy a
    // bare string" and "construct+parse a url" that they're not
    // comparable operations, despite similar wall-clock cost). And the
    // real regression this guards against (the old, unfixed abspath()) was
    // only ~30% slower than the fixed version on this sample set - well
    // within one noisy CI run's variance either way. A CI floor check
    // genuinely cannot distinguish "30% regression" from "unlucky
    // scheduling on a shared runner" reliably; pretending otherwise with a
    // tight threshold just trades false negatives for false positives.
    //
    // What this floor CAN catch reliably: something catastrophic (the fast
    // path's early-return removed entirely, an infinite loop, an
    // accidental O(n^2)) - multiple-x regressions, not incremental ones.
    // For anything more precise, run the dedicated, same-process A/B tool
    // instead of trusting CI: benchmarks/cpp/abspath_fix_verification/.
    double abspath_rate = ops_per_sec(
        [&] {
            for (auto& p : paths) {
                urlparser::url u(std::string("https://example.com") + p);
                sink += u.abspath().size();
            }
        },
        5000);
    all_ok &= check("url::abspath()", abspath_rate * paths.size(), 800'000.0);

    if (sink == 0xdeadbeefdeadbeefULL) return 1;  // keep sink live, never true
    if (!all_ok) {
        std::fprintf(stderr,
                      "\nPerformance regression guard FAILED. This floor is deliberately "
                      "generous (catches only gross regressions, several-x, not incremental "
                      "ones) - a failure here is a strong signal, not CI noise. Check recent "
                      "changes against OPTIMIZATION_NOTES.md.\n");
        return 1;
    }
    return 0;
}
