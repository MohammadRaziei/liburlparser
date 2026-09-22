// Benchmark: urlparser::idna::to_ascii (new feature, wraps ada) vs calling
// ada::idna::to_ascii directly - isolates the wrapper's own overhead in C++,
// where there is no language-binding boundary to cross (unlike the Python
// benchmark in bench_idna.py).
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "idna.h"
#include "ada/ada_idna.h"

int main() {
    std::vector<std::string> hosts;
    const char* corpus[] = {
        "caf\xc3\xa9.com",
        "m\xc3\xbcnchen.de",
        "www.7\xe2\x80\x91""Eleven.com",
        "\xd8\xaf\xd8\xa7\xd9\x85\xd9\x86\xd9\x87.\xd8\xa7\xdb\x8c\xd8\xb1\xd8\xa7\xd9\x86",
        "espa\xc3\xb1""a.es",
        "portugu\xc3\xaas.pt",
        "example.com",
        "xn--caf-dma.com",
    };
    for (int i = 0; i < 500; ++i)
        for (const char* h : corpus) hosts.push_back(h);

    const int repeats = 7;
    auto bench = [&](auto fn) {
        std::vector<double> times;
        for (int r = 0; r < repeats; ++r) {
            auto t0 = std::chrono::steady_clock::now();
            for (const auto& h : hosts) {
                volatile auto result = fn(h);
                (void)result;
            }
            auto t1 = std::chrono::steady_clock::now();
            times.push_back(std::chrono::duration<double>(t1 - t0).count());
        }
        return *std::min_element(times.begin(), times.end());
    };

    double t_wrapper = bench([](const std::string& h) {
        return urlparser::idna::to_ascii(h);
    });
    double t_raw = bench([](const std::string& h) {
        return ada::idna::to_ascii(h);
    });

    size_t n = hosts.size();
    std::printf("corpus size: %zu hostname lookups per run, %d runs\n\n", n, repeats);
    std::printf("urlparser::idna::to_ascii (new)   best=%8.3f ns/call\n", t_wrapper * 1e9 / n);
    std::printf("ada::idna::to_ascii (raw, direct) best=%8.3f ns/call\n", t_raw * 1e9 / n);
    std::printf("\nwrapper overhead: %+.2f%% (a thin pass-through call, as expected)\n",
                 (t_wrapper - t_raw) / t_raw * 100.0);
    return 0;
}
