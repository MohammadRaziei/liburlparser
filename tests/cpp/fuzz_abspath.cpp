// Randomized property test for url::abspath()'s dot-segment resolver.
//
// The 10,000-real-domain corpus (benchmarks/corpus/) and the handful of
// manual edge cases in test_url.cpp both check *plausible* paths. This
// generates thousands of adversarial-biased random paths (heavy on '.',
// '..', '/', repeated separators - exactly what the fast path's
// path_is_already_normalized() check has to get right) and checks
// abspath()'s output against an independent reference implementation,
// written fresh from the RFC 3986-style spec (segment-by-segment,
// unconditionally, no fast path, no caching) rather than reused from
// production code - the point is to catch a fast-path/slow-path
// disagreement, so the two must not share implementation.
//
// Not wired into CTest by default (see CMakeLists.txt: opt-in target,
// deterministic seed by default for reproducibility, but accepts a seed
// argument for actual fuzzing runs).
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

#include "urlparser.h"

namespace {

// Independent reference implementation: segment-by-segment, always via
// the general path, no fast-path shortcut, no cache. Deliberately
// reimplemented rather than copy-pasted from urlparser.cpp's
// path_is_already_normalized()-guarded version, so a bug in that fast
// path's *detection logic* (wrongly deciding a path is "already clean")
// has an independent implementation to disagree with.
std::string reference_abspath(const std::string& path) {
    std::string result;
    std::vector<size_t> segment_starts;

    if (!path.empty() && path[0] == '/') {
        result.push_back('/');
        segment_starts.push_back(0);
    }

    size_t previous = 0;
    size_t index = path.find('/');
    auto emit = [&](size_t start, size_t end) {
        if (end == start) return;
        std::string segment = path.substr(start, end - start);
        if (segment == ".") return;
        if (segment == "..") {
            if (segment_starts.size() > 1) {
                result.resize(segment_starts.back());
                segment_starts.pop_back();
            }
            return;
        }
        segment_starts.push_back(result.size());
        if (!result.empty() && result.back() != '/') result.push_back('/');
        result += segment;
    };
    for (; index != std::string::npos; previous = index + 1, index = path.find('/', index + 1)) emit(previous, index);
    emit(previous, path.size());
    return result;
}

std::string random_path(std::mt19937& rng) {
    // Heavily biased toward the characters that matter for this specific
    // algorithm (. / a) rather than uniform-random bytes - a fuzzer
    // hunting for a *logic* bug in segment resolution needs many
    // structurally-varied '.'/'..'/'/' arrangements far more than it
    // needs binary garbage (which a generic property test can't reason
    // about "correctness" for here anyway - there's no independent
    // reference for what arbitrary bytes "should" resolve to besides the
    // same algorithm, which would be circular).
    static const char alphabet[] = "..//a";
    std::uniform_int_distribution<size_t> len_dist(0, 40);
    std::uniform_int_distribution<size_t> char_dist(0, sizeof(alphabet) - 2);
    const size_t len = len_dist(rng);
    std::string s;
    s.reserve(len);
    for (size_t i = 0; i < len; ++i) s += alphabet[char_dist(rng)];
    return s;
}

}  // namespace

int main(int argc, char** argv) {
    const unsigned seed = argc > 1 ? static_cast<unsigned>(std::strtoul(argv[1], nullptr, 10)) : 42u;
    const int iterations = argc > 2 ? std::atoi(argv[2]) : 200000;

    std::mt19937 rng(seed);
    int mismatches = 0;

    for (int i = 0; i < iterations; ++i) {
        const std::string random_part = random_path(rng);
        // The URL parser always normalizes the path to start with '/'
        // (WHATWG behavior) before abspath() ever sees it - construct and
        // reference against the same '/'-prefixed text, or every mismatch
        // is just that normalization, not a real disagreement.
        const std::string path = "/" + random_part;
        urlparser::url u(std::string("https://example.com") + path);
        const std::string got = u.abspath();
        const std::string expected = reference_abspath(path);
        if (got != expected) {
            if (mismatches < 20) {
                std::fprintf(stderr, "MISMATCH  path=%-45s got=%-30s expected=%-30s\n", path.c_str(), got.c_str(),
                             expected.c_str());
            }
            mismatches++;
        }
    }

    std::printf("%d iterations (seed=%u), %d mismatches\n", iterations, seed, mismatches);
    return mismatches == 0 ? 0 : 1;
}
