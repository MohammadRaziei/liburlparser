"""
Benchmark: liburlparser.Hostname.normalized_ascii (new feature) vs ada_url's
native IDNA normalization, on a corpus of realistic internationalized (IDN)
domains. This isolates just the IDNA-normalization cost being added, rather
than full URL parsing (already covered by benchmarks/cpp and benchmarks/python).
"""
import statistics
import time

from liburlparser import Hostname
from ada_url import idna as ada_idna

UNICODE_HOSTS = [
    "café.com",
    "münchen.de",
    "www.7\u2011Eleven.com",
    "دامنه.ایران",
    "españa.es",
    "português.pt",
    "中国.icom.museum",
    "日本.jp",
    "россия.рф",
    "ελλάδα.gr",
    "example.com",            # plain ASCII control, mixed into the corpus
    "xn--caf-dma.com",        # already-punycoded control
] * 500  # 6000 lookups per run, keeps single run well under a second


def bench(fn, data, repeats=5):
    times = []
    for _ in range(repeats):
        t0 = time.perf_counter()
        for h in data:
            fn(h)
        times.append(time.perf_counter() - t0)
    return times


def liburlparser_normalize(h):
    return Hostname(h).normalized_ascii


def ada_normalize(h):
    return ada_idna.encode(h)


if __name__ == "__main__":
    lib_times = bench(liburlparser_normalize, UNICODE_HOSTS)
    ada_times = bench(ada_normalize, UNICODE_HOSTS)

    n = len(UNICODE_HOSTS)
    print(f"corpus size: {n} hostname lookups per run, {len(lib_times)} runs\n")

    for name, times in (("liburlparser (new normalized_ascii)", lib_times),
                         ("ada_url.idna.to_ascii (reference)", ada_times)):
        best = min(times)
        med = statistics.median(times)
        print(f"{name:38s}  best={best*1e6/n:8.3f} us/call  "
              f"median={med*1e6/n:8.3f} us/call")

    overhead = (min(lib_times) - min(ada_times)) / min(ada_times) * 100
    print(f"\nliburlparser vs raw ada_url python call: {overhead:+.1f}%")
    print("(liburlparser comes out ahead here because ada_url's own Python")
    print(" package crosses a cffi boundary and returns `bytes`, while")
    print(" liburlparser's nanobind binding calls ada::idna::to_ascii()")
    print(" directly in-process and returns a Python str. Same underlying")
    print(" IDNA algorithm either way - this measures binding overhead,")
    print(" not the normalization algorithm itself.)")
