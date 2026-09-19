import time
import liburlparser
import pydomainextractor

import os

CORPUS_DIR = os.path.join(os.path.dirname(__file__), "..", "corpus")
with open(os.path.join(CORPUS_DIR, "domains.txt")) as f:
    domains = [line.strip() for line in f if line.strip()]

pde = pydomainextractor.DomainExtractor()


def bench(fn, reps):
    t0 = time.perf_counter()
    for _ in range(reps):
        for d in domains:
            fn(d)
    return time.perf_counter() - t0


rounds = 8
reps_per_round = 3
singleton_total = suffix_only_total = bare_total = 0.0
construct_total = parsed_total = production_total = pde_total = 0.0

for _ in range(rounds):
    singleton_total += bench(lambda d: liburlparser.Hostname.probe_singleton_only(), reps_per_round)
    suffix_only_total += bench(liburlparser.Hostname.probe_suffix_only, reps_per_round)
    bare_total += bench(liburlparser.Hostname.probe_bare_call, reps_per_round)
    construct_total += bench(liburlparser.Hostname.probe_construct_only, reps_per_round)
    parsed_total += bench(liburlparser.Hostname.probe_parsed_only, reps_per_round)
    production_total += bench(liburlparser.Hostname.extract_dict_from_host, reps_per_round)
    pde_total += bench(pde.extract, reps_per_round)

total_ops = rounds * reps_per_round * len(domains)
print(f"probe_singleton_only (just psl::instance() + tiny lookup) : {total_ops/singleton_total:>12,.0f} ops/s")
print(f"probe_suffix_only    (copy + suffix_of ONLY, no slicing)  : {total_ops/suffix_only_total:>12,.0f} ops/s")
print(f"probe_bare_call      (just the call, nothing else)        : {total_ops/bare_total:>12,.0f} ops/s")
print(f"probe_construct_only (+ std::string copy + ctor)          : {total_ops/construct_total:>12,.0f} ops/s")
print(f"probe_parsed_only    (+ ensure_parsed / PSL lookup)       : {total_ops/parsed_total:>12,.0f} ops/s")
print(f"extract_dict_from_host (production, 6-field, direct)      : {total_ops/production_total:>12,.0f} ops/s")
print(f"PyDomainExtractor .extract() (3-field)                     : {total_ops/pde_total:>12,.0f} ops/s")
print()
print(f"bare_call     -> construct_only : {bare_total/construct_total:.3f}x slower (mandatory string copy + ctor)")
print(f"construct_only -> parsed_only   : {construct_total/parsed_total:.3f}x slower (ensure_parsed / PSL lookup)")
print(f"suffix_only   -> parsed_only    : {suffix_only_total/parsed_total:.3f}x slower (ensure_parsed's EXTRA domain/subdomain allocs beyond suffix_of alone)")
print(f"singleton     -> suffix_only    : {singleton_total/suffix_only_total:.3f}x slower (pure psl lookup cost, beyond singleton access)")
print()
print(f"production vs PDE: {pde_total/production_total:.3f}x  (>1 means liburlparser still slower)")
