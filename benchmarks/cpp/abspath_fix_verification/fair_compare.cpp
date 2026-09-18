#include "urlparser_new.h"
#include "urlparser_old.h"
#include <ada.h>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <vector>
#include <string>

int main() {
    std::ifstream f("urls.txt");
    std::vector<std::string> urls;
    std::string line;
    while (std::getline(f, line)) if (!line.empty()) urls.push_back(line);

    volatile size_t sink = 0;

    auto run_old = [&](int reps) {
        for (int r = 0; r < reps; ++r)
            for (auto& u : urls) {
                try {
                    urlparser_old::url parsed(u);
                    auto p = parsed.protocol(); auto h = parsed.host_text();
                    auto path = parsed.abspath(); auto q = parsed.query(); auto frag = parsed.fragment();
                    sink += p.size() + h.size() + path.size() + q.size() + frag.size();
                } catch (...) {}
            }
    };
    auto run_new = [&](int reps) {
        for (int r = 0; r < reps; ++r)
            for (auto& u : urls) {
                try {
                    urlparser::url parsed(u);
                    auto p = parsed.protocol(); auto h = parsed.host_text();
                    const auto& path = parsed.abspath(); auto q = parsed.query(); auto frag = parsed.fragment();
                    sink += p.size() + h.size() + path.size() + q.size() + frag.size();
                } catch (...) {}
            }
    };
    auto run_ada = [&](int reps) {
        for (int r = 0; r < reps; ++r)
            for (auto& u : urls) {
                auto result = ada::parse<ada::url_aggregator>(u);
                if (!result) continue;
                auto p = result->get_protocol(); auto h = result->get_host();
                auto path = result->get_pathname(); auto q = result->get_search(); auto frag = result->get_hash();
                sink += p.size() + h.size() + path.size() + q.size() + frag.size();
            }
    };

    // Interleaved: N short rounds, each measuring old/new/ada back-to-back,
    // so any slow drift in the environment (thermal, scheduler noise) hits
    // all three roughly equally instead of biasing whichever ran last.
    constexpr int rounds = 12;
    constexpr int reps_per_round = 4;
    double old_total = 0, new_total = 0, ada_total = 0;

    for (int round = 0; round < rounds; ++round) {
        auto t0 = std::chrono::steady_clock::now();
        run_old(reps_per_round);
        auto t1 = std::chrono::steady_clock::now();
        run_new(reps_per_round);
        auto t2 = std::chrono::steady_clock::now();
        run_ada(reps_per_round);
        auto t3 = std::chrono::steady_clock::now();

        old_total += std::chrono::duration<double>(t1 - t0).count();
        new_total += std::chrono::duration<double>(t2 - t1).count();
        ada_total += std::chrono::duration<double>(t3 - t2).count();
    }

    double total_ops = double(rounds) * double(reps_per_round) * double(urls.size());
    std::printf("liburlparser OLD (unfixed abspath): %14.0f ops/s\n", total_ops / old_total);
    std::printf("liburlparser NEW (fixed abspath):    %14.0f ops/s\n", total_ops / new_total);
    std::printf("ada:                                  %14.0f ops/s\n", total_ops / ada_total);
    std::printf("\nnew vs old: %.3fx\n", old_total / new_total);
    std::printf("new vs ada: %.3fx  (>1 means liburlparser is faster)\n", ada_total / new_total);
    std::printf("old vs ada: %.3fx  (>1 means liburlparser was already faster)\n", ada_total / old_total);

    return sink == 0xdeadbeef ? 1 : 0;
}
