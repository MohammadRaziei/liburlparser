// Honest, standalone head-to-head: the *actual* algorithm shipped in
// liburlparser (ankerl::unordered_dense::map, shrinking-search) vs a real,
// arena-allocated trie (contiguous storage, sorted-vector children,
// binary_search per level - not per-node heap hash maps like
// PyDomainExtractor's Rust AHashMap-of-AHashMap).
//
// Same PSL file, same 10,000-domain corpus, same machine, same run.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "../../../src/ankerl/unordered_dense.h"

static inline char ascii_tolower(char c) { return (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : c; }

static size_t segment_count(const std::string& text) {
    size_t count = 1;
    size_t pos = text.find('.');
    while (pos != std::string::npos) {
        count += 1;
        pos = text.find('.', pos + 1);
    }
    return count;
}

// ============================================================================
// 1) Current shipped approach: flat ankerl hash map, reversed-string keys,
//    shrinking search, bounded by max_length/max_depth.
//    (Copied verbatim from src/urlparser.cpp's suffix_table/add_rule/suffix_of.)
// ============================================================================
struct suffix_table_hash {
    ankerl::unordered_dense::map<std::string, size_t> data;
    size_t max_length = 0;
    size_t max_depth = 0;
};

static void add_rule_hash(suffix_table_hash& t, std::string& rule, int level_adjust, size_t trim) {
    std::string copy(rule.rbegin(), rule.rend() - trim);
    size_t length = segment_count(copy) + level_adjust;
    if (copy.size() > t.max_length) t.max_length = copy.size();
    if (length > t.max_depth) t.max_depth = length;
    t.data[std::move(copy)] = length;
}

static std::string suffix_of_hash(const suffix_table_hash& t, const std::string& hostname_text) {
    std::string tld(hostname_text.rbegin(), hostname_text.rend());
    std::transform(tld.begin(), tld.end(), tld.begin(), ascii_tolower);

    if (t.max_length > 0 && tld.size() > t.max_length) {
        const size_t scan_limit = std::min(tld.size(), t.max_length);
        size_t dot_count = 0, last_dot = std::string::npos, cut = std::string::npos;
        for (size_t i = 0; i < scan_limit; ++i) {
            if (tld[i] == '.') {
                last_dot = i;
                if (t.max_depth > 0 && ++dot_count == t.max_depth) { cut = i; break; }
            }
        }
        if (cut == std::string::npos) cut = (tld[t.max_length] == '.') ? t.max_length : last_dot;
        tld.resize((cut == std::string::npos || cut == 0) ? 0 : cut);
    }

    while (!tld.empty()) {
        if (tld.size() <= t.max_length) {
            auto it = t.data.find(tld);
            if (it != t.data.end()) {
                std::reverse(tld.begin(), tld.end());
                return tld;
            }
        }
        size_t position = tld.rfind('.');
        tld.resize((position == std::string::npos || position == 0) ? 0 : position);
    }
    const size_t last_dot = hostname_text.rfind('.');
    std::string result = (last_dot == std::string::npos) ? hostname_text : hostname_text.substr(last_dot + 1);
    std::transform(result.begin(), result.end(), result.begin(), ascii_tolower);
    return result;
}

// ============================================================================
// 2) A *real* trie: one contiguous arena of nodes (no per-node heap map),
//    children stored as a sorted vector<pair<label,child_index>>, looked up
//    with binary_search. Each hostname label is touched exactly once -
//    no re-hashing of shrinking substrings.
// ============================================================================
struct trie_node {
    std::vector<std::pair<std::string, int>> children;  // sorted by label
    std::vector<std::string> exceptions;                 // sorted, for "!label" rules
    bool is_end = false;
    bool is_wildcard = false;
};

struct trie_table {
    std::vector<trie_node> arena;
    trie_table() { arena.emplace_back(); }  // index 0 = root

    int get_or_create_child(int node_idx, const std::string& label) {
        auto cmp = [](const std::pair<std::string, int>& p, const std::string& l) { return p.first < l; };
        auto& kids = arena[node_idx].children;
        auto it = std::lower_bound(kids.begin(), kids.end(), label, cmp);
        if (it != kids.end() && it->first == label) return it->second;
        int new_idx = static_cast<int>(arena.size());
        arena.emplace_back();
        // arena may have reallocated - re-fetch children vector via node_idx
        auto& kids2 = arena[node_idx].children;
        auto it2 = std::lower_bound(kids2.begin(), kids2.end(), label, cmp);
        kids2.insert(it2, {label, new_idx});
        return new_idx;
    }

    const int* find_child(int node_idx, const std::string& label) const {
        auto cmp = [](const std::pair<std::string, int>& p, const std::string& l) { return p.first < l; };
        const auto& kids = arena[node_idx].children;
        auto it = std::lower_bound(kids.begin(), kids.end(), label, cmp);
        if (it != kids.end() && it->first == label) return &it->second;
        return nullptr;
    }
};

// ============================================================================
// 2b) Same arena-trie shape, but each node's children is the *same*
//     ankerl::unordered_dense::map used by the shipped implementation,
//     instead of a sorted vector. Answers directly: "does swapping in the
//     project's own dense map at every trie level change the outcome?"
// ============================================================================
struct trie_node_dense {
    ankerl::unordered_dense::map<std::string, int> children;
    std::vector<std::string> exceptions;
    bool is_end = false;
    bool is_wildcard = false;
};

struct trie_table_dense {
    std::vector<trie_node_dense> arena;
    trie_table_dense() { arena.emplace_back(); }

    int get_or_create_child(int node_idx, const std::string& label) {
        auto& kids = arena[node_idx].children;
        auto it = kids.find(label);
        if (it != kids.end()) return it->second;
        int new_idx = static_cast<int>(arena.size());
        arena.emplace_back();
        arena[node_idx].children[label] = new_idx;  // re-index node_idx after emplace_back
        return new_idx;
    }

    const int* find_child(int node_idx, const std::string& label) const {
        const auto& kids = arena[node_idx].children;
        auto it = kids.find(label);
        return it != kids.end() ? &it->second : nullptr;
    }
};

// ============================================================================
// 3) The user's refined design: outer ankerl::unordered_dense::map keyed by
//    the tld label (e.g. "com", "uk"), value = a struct holding an
//    ankerl::unordered_dense::set of "everything below the tld" for deeper
//    rules (reversed, same encoding the shipped map already uses) PLUS
//    max_length/max_depth *scoped to that one tld's own subtree*.
//
//    Why this can win: for TLDs with zero deeper PSL rules (the overwhelming
//    majority of real traffic - plain .com/.net/.org/... registrations),
//    `rest` is empty, so the lookup returns after exactly ONE hash lookup -
//    no shrinking loop at all. Only TLDs that actually have deep rules
//    (.uk, .jp, amazonaws.com, ...) pay for a (much smaller, tightly
//    bounded) shrinking search - scoped to *that tld's own* max_length/
//    max_depth instead of the global worst case across all 9,766 rules.
// ============================================================================
struct transparent_str_hash {
    using is_transparent = void;
    using is_avalanching = void;
    uint64_t operator()(std::string_view sv) const noexcept {
        return ankerl::unordered_dense::hash<std::string_view>{}(sv);
    }
};

struct suffix_bucket {
    // transparent hash + std::equal_to<> => .find()/.contains() accept a
    // string_view directly, no temporary std::string built per lookup.
    ankerl::unordered_dense::set<std::string, transparent_str_hash, std::equal_to<>> rest;
    size_t max_length = 0;  // longest entry in `rest`, 0 if no deeper rules
    size_t max_depth = 0;   // most extra labels in `rest`, 0 if no deeper rules
    bool tld_is_suffix = false;  // whether the bare tld label itself is a listed rule
};

struct suffix_table_ankerl {
    ankerl::unordered_dense::map<std::string, suffix_bucket, transparent_str_hash, std::equal_to<>> buckets;
};

static void add_rule_ankerl(suffix_table_ankerl& t, std::string& rule, int level_adjust, size_t trim) {
    // Identical key construction to the shipped add_rule_hash: reversed,
    // trimmed rule string - only now we split it into "tld bucket key" +
    // "rest", instead of storing the whole thing as one flat map key.
    std::string copy(rule.rbegin(), rule.rend() - trim);
    size_t dot = copy.find('.');
    std::string tld_key = (dot == std::string::npos) ? copy : copy.substr(0, dot);

    auto& bucket = t.buckets[tld_key];
    if (dot == std::string::npos) {
        bucket.tld_is_suffix = true;
        return;
    }
    std::string rest = copy.substr(dot + 1);
    size_t length = segment_count(rest) + level_adjust;
    if (rest.size() > bucket.max_length) bucket.max_length = rest.size();
    if (length > bucket.max_depth) bucket.max_depth = length;
    bucket.rest.insert(std::move(rest));
}

static std::string suffix_of_ankerl(const suffix_table_ankerl& t, const std::string& hostname_text) {
    std::string full(hostname_text.rbegin(), hostname_text.rend());
    std::transform(full.begin(), full.end(), full.begin(), ascii_tolower);

    const size_t first_dot = full.find('.');
    const std::string tld_key = (first_dot == std::string::npos) ? full : full.substr(0, first_dot);

    auto it = t.buckets.find(tld_key);
    if (it == t.buckets.end()) {
        const size_t last_dot = hostname_text.rfind('.');
        std::string result = (last_dot == std::string::npos) ? hostname_text : hostname_text.substr(last_dot + 1);
        std::transform(result.begin(), result.end(), result.begin(), ascii_tolower);
        return result;
    }
    const suffix_bucket& bucket = it->second;

    // Fast path: no deeper rules under this tld at all (true for the bulk
    // of real-world .com/.net/.org/... registrations) - one hash lookup,
    // done, no shrinking loop.
    if (first_dot == std::string::npos || bucket.rest.empty()) {
        std::string result = tld_key;
        std::reverse(result.begin(), result.end());
        return result;
    }

    // Deeper rules exist under this tld - shrinking search, bounded by
    // *this bucket's own* max_length/max_depth (far tighter than the
    // global worst case across all 9,766 rules).
    std::string tail = full.substr(first_dot + 1);
    if (bucket.max_length > 0 && tail.size() > bucket.max_length) {
        const size_t scan_limit = std::min(tail.size(), bucket.max_length);
        size_t dot_count = 0, last_dot = std::string::npos, cut = std::string::npos;
        for (size_t i = 0; i < scan_limit; ++i) {
            if (tail[i] == '.') {
                last_dot = i;
                if (bucket.max_depth > 0 && ++dot_count == bucket.max_depth) { cut = i; break; }
            }
        }
        if (cut == std::string::npos) cut = (tail[bucket.max_length] == '.') ? bucket.max_length : last_dot;
        tail.resize((cut == std::string::npos || cut == 0) ? 0 : cut);
    }

    while (!tail.empty()) {
        if (tail.size() <= bucket.max_length && bucket.rest.contains(tail)) {
            // tail and tld_key are each individually reversed segments; to
            // get back the normal-order string we must reverse each piece
            // back on its own and THEN join them in normal order - reversing
            // the concatenation as a whole would also swap segment order.
            std::string rest_normal(tail.rbegin(), tail.rend());
            std::string tld_normal(tld_key.rbegin(), tld_key.rend());
            return rest_normal + "." + tld_normal;
        }
        size_t position = tail.rfind('.');
        tail.resize((position == std::string::npos || position == 0) ? 0 : position);
    }
    std::string result = tld_key;
    std::reverse(result.begin(), result.end());
    return result;
}

// ----------------------------------------------------------------------------
// Same bucketed lookup, but rebuilt around ownership instead of ad-hoc
// copies: the hostname is taken BY VALUE (so a caller passing an rvalue
// lets us move it in for free), reversed/lowercased ONCE in place inside
// that single owned buffer, and every subsequent slice (tld_key, tail) is a
// std::string_view into that SAME owned buffer - zero further allocation.
// This is safe precisely because the buffer we're viewing into is a local
// we own for the full lifetime of the call: it can never dangle, unlike a
// string_view built over a string the *caller* still controls.
// Only the final match copies bytes, to produce the returned std::string.
// ----------------------------------------------------------------------------
static std::string suffix_of_ankerl_owned(const suffix_table_ankerl& t, std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), ascii_tolower);
    std::reverse(text.begin(), text.end());  // in place, zero extra allocation
    const std::string_view view(text);       // safe: `text` outlives every use of `view` below

    const size_t first_dot = view.find('.');
    const std::string_view tld_key = (first_dot == std::string_view::npos) ? view : view.substr(0, first_dot);

    auto it = t.buckets.find(tld_key);  // transparent lookup - no temporary std::string built
    if (it == t.buckets.end()) {
        std::string result(tld_key.rbegin(), tld_key.rend());  // first label of reversed == last label of original
        return result;
    }
    const suffix_bucket& bucket = it->second;

    if (first_dot == std::string_view::npos || bucket.rest.empty()) {
        return std::string(tld_key.rbegin(), tld_key.rend());
    }

    std::string_view tail = view.substr(first_dot + 1);
    if (bucket.max_length > 0 && tail.size() > bucket.max_length) {
        const size_t scan_limit = std::min(tail.size(), bucket.max_length);
        size_t dot_count = 0, last_dot = std::string_view::npos, cut = std::string_view::npos;
        for (size_t i = 0; i < scan_limit; ++i) {
            if (tail[i] == '.') {
                last_dot = i;
                if (bucket.max_depth > 0 && ++dot_count == bucket.max_depth) { cut = i; break; }
            }
        }
        if (cut == std::string_view::npos) cut = (tail[bucket.max_length] == '.') ? bucket.max_length : last_dot;
        tail = tail.substr(0, (cut == std::string_view::npos || cut == 0) ? 0 : cut);
    }

    while (!tail.empty()) {
        if (tail.size() <= bucket.max_length && bucket.rest.contains(tail)) {
            std::string rest_normal(tail.rbegin(), tail.rend());
            std::string tld_normal(tld_key.rbegin(), tld_key.rend());
            return rest_normal + "." + tld_normal;
        }
        const size_t position = tail.rfind('.');
        tail = tail.substr(0, (position == std::string_view::npos || position == 0) ? 0 : position);
    }
    return std::string(tld_key.rbegin(), tld_key.rend());
}

// ============================================================================
// v3: the storage side goes fully string_view too. `psl_text` is ONE owned
// buffer (would live as a member of the real `psl` class) holding every
// rule's reversed bytes back to back, built once at load time. Every map/
// set key is a std::string_view slice into that single buffer - no per-rule
// std::string at all, not even a short-string-optimized one. Query-side
// hostnames still get their own small owned+reversed buffer per call
// (unavoidable - arbitrary caller input, we don't control its lifetime),
// exactly like suffix_of_ankerl_owned above.
//
// Safety note: a std::string's move constructor is guaranteed not to
// reallocate its heap buffer (only pointer/size/capacity are transferred),
// so offsets recorded against the staging buffer stay valid once it is
// moved into the table's psl_text member - the character data never moves.
// ============================================================================
// ============================================================================
// Shared PSL parsing (mirrors urlparser.cpp's psl::psl(istream) line handling)
// ============================================================================
template <class OnRule>
static void parse_psl(std::istream& stream, OnRule on_rule) {
    std::string line;
    while (std::getline(stream, line)) {
        auto it = std::find_if(line.begin(), line.end(), [](char c) { return std::isspace((unsigned char)c); });
        line.resize(it - line.begin());
        if (line.empty()) continue;
        if (line.compare(0, 2, "//") == 0) continue;
        std::transform(line.begin(), line.end(), line.begin(), ascii_tolower);

        if (line[0] == '*') {
            if (line.size() <= 2 || line[1] != '.') continue;
            on_rule(line, 1, 2);
        } else if (line[0] == '!') {
            if (line.size() <= 1) continue;
            on_rule(line, -1, 1);
        } else {
            on_rule(line, 0, 0);
        }
    }
}

struct suffix_bucket_sv {
    ankerl::unordered_dense::set<std::string_view> rest;
    size_t max_length = 0;
    size_t max_depth = 0;
    bool tld_is_suffix = false;
};

struct suffix_table_sv {
    std::string psl_text;  // owns every rule's reversed bytes; everything below only views into this
    ankerl::unordered_dense::map<std::string_view, suffix_bucket_sv> buckets;
};

static size_t segment_count_sv(std::string_view text) {
    size_t count = 1;
    size_t pos = text.find('.');
    while (pos != std::string_view::npos) {
        count += 1;
        pos = text.find('.', pos + 1);
    }
    return count;
}

static void build_suffix_table_sv(suffix_table_sv& t, const std::string& psl_raw) {
    struct rule_record { size_t offset; size_t length; int level_adjust; };
    std::vector<rule_record> records;
    std::string staging;
    staging.reserve(psl_raw.size());

    std::stringstream s(psl_raw);
    parse_psl(s, [&](std::string& line, int level_adjust, size_t trim) {
        const size_t start = staging.size();
        staging.append(line.rbegin(), line.rend() - trim);  // reversed bytes, appended - no separate alloc per rule
        records.push_back({start, staging.size() - start, level_adjust});
    });

    t.psl_text = std::move(staging);  // pointer takeover only - offsets above stay valid

    for (auto& r : records) {
        std::string_view copy_view(t.psl_text.data() + r.offset, r.length);
        const size_t dot = copy_view.find('.');
        std::string_view tld_key = (dot == std::string_view::npos) ? copy_view : copy_view.substr(0, dot);

        auto& bucket = t.buckets[tld_key];
        if (dot == std::string_view::npos) {
            bucket.tld_is_suffix = true;
            continue;
        }
        std::string_view rest = copy_view.substr(dot + 1);
        const size_t length = segment_count_sv(rest) + r.level_adjust;
        if (rest.size() > bucket.max_length) bucket.max_length = rest.size();
        if (length > bucket.max_depth) bucket.max_depth = length;
        bucket.rest.insert(rest);
    }
}

static std::string suffix_of_sv(const suffix_table_sv& t, std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), ascii_tolower);
    std::reverse(text.begin(), text.end());
    const std::string_view view(text);

    const size_t first_dot = view.find('.');
    const std::string_view tld_key = (first_dot == std::string_view::npos) ? view : view.substr(0, first_dot);

    auto it = t.buckets.find(tld_key);  // Key IS string_view already - no transparent-hash machinery needed
    if (it == t.buckets.end()) return std::string(tld_key.rbegin(), tld_key.rend());
    const suffix_bucket_sv& bucket = it->second;

    if (first_dot == std::string_view::npos || bucket.rest.empty()) {
        return std::string(tld_key.rbegin(), tld_key.rend());
    }

    std::string_view tail = view.substr(first_dot + 1);
    if (bucket.max_length > 0 && tail.size() > bucket.max_length) {
        const size_t scan_limit = std::min(tail.size(), bucket.max_length);
        size_t dot_count = 0, last_dot = std::string_view::npos, cut = std::string_view::npos;
        for (size_t i = 0; i < scan_limit; ++i) {
            if (tail[i] == '.') {
                last_dot = i;
                if (bucket.max_depth > 0 && ++dot_count == bucket.max_depth) { cut = i; break; }
            }
        }
        if (cut == std::string_view::npos) cut = (tail[bucket.max_length] == '.') ? bucket.max_length : last_dot;
        tail = tail.substr(0, (cut == std::string_view::npos || cut == 0) ? 0 : cut);
    }

    while (!tail.empty()) {
        if (tail.size() <= bucket.max_length && bucket.rest.contains(tail)) {
            std::string rest_normal(tail.rbegin(), tail.rend());
            std::string tld_normal(tld_key.rbegin(), tld_key.rend());
            return rest_normal + "." + tld_normal;
        }
        const size_t position = tail.rfind('.');
        tail = tail.substr(0, (position == std::string_view::npos || position == 0) ? 0 : position);
    }
    return std::string(tld_key.rbegin(), tld_key.rend());
}

static std::vector<std::string> split_labels(const std::string& text) {
    std::vector<std::string> labels;
    size_t start = 0;
    while (true) {
        size_t dot = text.find('.', start);
        if (dot == std::string::npos) {
            labels.push_back(text.substr(start));
            break;
        }
        labels.push_back(text.substr(start, dot - start));
        start = dot + 1;
    }
    return labels;
}

// rule: already lowercased, WITHOUT the leading "*." / "!" (trim applied by caller).
// level_adjust: 0 normal, +1 wildcard ("*.foo.bar"), -1 exception ("!foo.bar").
// Templated on the trie type so the exact same logic drives both the
// sorted-vector trie and the dense-map trie - no chance of the two
// implementations silently drifting apart.
template <class Trie>
static void add_rule_trie(Trie& t, const std::string& rule, int level_adjust) {
    auto labels = split_labels(rule);
    if (level_adjust == -1) {
        int node_idx = 0;
        for (size_t i = labels.size(); i-- > 1;) node_idx = t.get_or_create_child(node_idx, labels[i]);
        auto& exc = t.arena[node_idx].exceptions;
        auto it = std::lower_bound(exc.begin(), exc.end(), labels[0]);
        if (it == exc.end() || *it != labels[0]) exc.insert(it, labels[0]);
        int leaf = t.get_or_create_child(node_idx, labels[0]);
        t.arena[leaf].is_end = true;
        return;
    }
    int node_idx = 0;
    for (size_t i = labels.size(); i-- > 0;) node_idx = t.get_or_create_child(node_idx, labels[i]);
    t.arena[node_idx].is_end = true;
    if (level_adjust == 1) t.arena[node_idx].is_wildcard = true;
}

// Longest public-suffix match, walking the hostname's labels right-to-left,
// touching each label exactly once (no re-hashing of shrinking substrings).
// Templated for the same reason as add_rule_trie above.
template <class Trie>
static std::string suffix_of_trie(const Trie& t, const std::string& hostname_text) {
    std::string lower = hostname_text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ascii_tolower);
    auto labels = split_labels(lower);

    int node_idx = 0;
    size_t matched_labels = 0;
    size_t best_end_labels = 0;

    size_t i = labels.size();
    while (i-- > 0) {
        const int* child = t.find_child(node_idx, labels[i]);
        if (!child) break;
        node_idx = *child;
        matched_labels += 1;

        if (t.arena[node_idx].is_wildcard) {
            if (i > 0) {
                const auto& exc = t.arena[node_idx].exceptions;
                bool is_exception = std::binary_search(exc.begin(), exc.end(), labels[i - 1]);
                if (!is_exception) matched_labels += 1;
            }
            best_end_labels = matched_labels;
            break;
        }
        if (t.arena[node_idx].is_end) best_end_labels = matched_labels;
    }

    if (best_end_labels == 0) {
        const size_t last_dot = hostname_text.rfind('.');
        std::string result = (last_dot == std::string::npos) ? hostname_text : hostname_text.substr(last_dot + 1);
        std::transform(result.begin(), result.end(), result.begin(), ascii_tolower);
        return result;
    }

    std::string result;
    for (size_t k = labels.size() - best_end_labels; k < labels.size(); ++k) {
        if (!result.empty()) result += '.';
        result += labels[k];
    }
    return result;
}

int main(int argc, char** argv) {
    // Defaults assume this binary is run from the repo root; override with
    // explicit paths if you're running it from elsewhere:
    //   ./psl_structure_comparison <public_suffix_list.dat> <domains.txt>
    std::string psl_path = argc > 1 ? argv[1] : "src/public_suffix_list.dat";
    std::string domains_path = argc > 2 ? argv[2] : "benchmarks/corpus/domains.txt";

    // ---- Load real PSL (~9,766 rules) ----
    std::ifstream psl_file(psl_path);
    if (!psl_file.good()) {
        std::cerr << "cannot open " << psl_path << " (pass its path as argv[1])\n";
        return 1;
    }
    std::stringstream psl_buf;
    psl_buf << psl_file.rdbuf();
    std::string psl_text = psl_buf.str();

    suffix_table_hash hash_table;
    {
        std::stringstream s(psl_text);
        parse_psl(s, [&](std::string& line, int level_adjust, size_t trim) {
            add_rule_hash(hash_table, line, level_adjust, trim);
        });
    }

    trie_table trie;
    size_t rule_count = 0;
    {
        std::stringstream s(psl_text);
        parse_psl(s, [&](std::string& line, int level_adjust, size_t trim) {
            add_rule_trie(trie, line.substr(trim), level_adjust);
            rule_count++;
        });
    }

    trie_table_dense trie_dense;
    {
        std::stringstream s(psl_text);
        parse_psl(s, [&](std::string& line, int level_adjust, size_t trim) {
            add_rule_trie(trie_dense, line.substr(trim), level_adjust);
        });
    }

    suffix_table_ankerl bucketed;
    {
        std::stringstream s(psl_text);
        parse_psl(s, [&](std::string& line, int level_adjust, size_t trim) {
            add_rule_ankerl(bucketed, line, level_adjust, trim);
        });
    }

    suffix_table_sv bucketed_sv;
    build_suffix_table_sv(bucketed_sv, psl_text);

    // ---- Load real domain corpus (10,000 real domains) ----
    std::ifstream dom_file(domains_path);
    if (!dom_file.good()) {
        std::cerr << "cannot open " << domains_path << " (pass its path as argv[2])\n";
        return 1;
    }
    std::vector<std::string> domains;
    {
        std::string line;
        while (std::getline(dom_file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) domains.push_back(line);
        }
    }

    std::cout << "PSL rules loaded: " << rule_count << "\n";
    std::cout << "Trie (vector) arena nodes: " << trie.arena.size() << "\n";
    std::cout << "Trie (dense_map) arena nodes: " << trie_dense.arena.size() << "\n";
    std::cout << "Hash map entries: " << hash_table.data.size() << "\n";
    std::cout << "Bucketed map: " << bucketed.buckets.size() << " tld buckets\n";
    std::cout << "Bucketed (string_view) map: " << bucketed_sv.buckets.size() << " tld buckets, psl_text = "
              << bucketed_sv.psl_text.size() << " bytes\n";
    std::cout << "Domains: " << domains.size() << "\n\n";

    // ---- Correctness check: all six must agree on every domain ----
    size_t mismatches = 0;
    for (auto& d : domains) {
        std::string a = suffix_of_hash(hash_table, d);
        std::string b = suffix_of_trie(trie, d);
        std::string c = suffix_of_trie(trie_dense, d);
        std::string e = suffix_of_ankerl(bucketed, d);
        std::string f = suffix_of_ankerl_owned(bucketed, d);  // takes a copy of d internally
        std::string g = suffix_of_sv(bucketed_sv, d);         // takes a copy of d internally
        if (a != b || a != c || a != e || a != f || a != g) {
            if (mismatches < 10)
                std::cerr << "MISMATCH  " << d << "  hash=" << a << "  trie_vec=" << b << "  trie_dense=" << c
                           << "  bucketed=" << e << "  bucketed_owned=" << f << "  sv=" << g << "\n";
            mismatches++;
        }
    }
    std::cout << "Mismatches: " << mismatches << " / " << domains.size() << "\n\n";

    // ---- Why doesn't a cheaper 2nd search lower the total? Measure it. ----
    {
        auto it_com = bucketed.buckets.find(std::string("moc"));  // reversed "com"
        size_t com_rest_size = (it_com != bucketed.buckets.end()) ? it_com->second.rest.size() : 0;
        std::cout << "'.com' bucket alone holds " << com_rest_size
                   << " deeper (private-suffix) entries out of " << hash_table.data.size() << " total rules\n";

        size_t hit_empty_rest = 0, hit_nonempty_rest = 0, unknown_tld = 0;
        for (auto& d : domains) {
            std::string full(d.rbegin(), d.rend());
            std::transform(full.begin(), full.end(), full.begin(), ascii_tolower);
            size_t first_dot = full.find('.');
            std::string tld_key = (first_dot == std::string::npos) ? full : full.substr(0, first_dot);
            auto it = bucketed.buckets.find(tld_key);
            if (it == bucketed.buckets.end()) { unknown_tld++; continue; }
            if (it->second.rest.empty()) hit_empty_rest++; else hit_nonempty_rest++;
        }
        std::cout << "domains whose tld bucket has NO deeper rules (1 lookup, fast path): " << hit_empty_rest << " / " << domains.size() << "\n";
        std::cout << "domains whose tld bucket HAS deeper rules (must also search `rest`): " << hit_nonempty_rest << " / " << domains.size() << "\n\n";
    }

    // ---- Timed benchmark, x50 over the corpus ----
    constexpr int reps = 50;
    volatile size_t sink = 0;

    auto t0 = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r)
        for (auto& d : domains) sink += suffix_of_hash(hash_table, d).size();
    auto t1 = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r)
        for (auto& d : domains) sink += suffix_of_trie(trie, d).size();
    auto t2 = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r)
        for (auto& d : domains) sink += suffix_of_trie(trie_dense, d).size();
    auto t3 = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r)
        for (auto& d : domains) sink += suffix_of_ankerl(bucketed, d).size();
    auto t4 = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r)
        for (auto& d : domains) sink += suffix_of_ankerl_owned(bucketed, d).size();  // copies d in (by value)
    auto t5 = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r)
        for (auto& d : domains) sink += suffix_of_sv(bucketed_sv, d).size();  // copies d in (by value)
    auto t6 = std::chrono::steady_clock::now();

    double hash_s = std::chrono::duration<double>(t1 - t0).count();
    double trie_s = std::chrono::duration<double>(t2 - t1).count();
    double trie_dense_s = std::chrono::duration<double>(t3 - t2).count();
    double bucketed_s = std::chrono::duration<double>(t4 - t3).count();
    double bucketed_owned_s = std::chrono::duration<double>(t5 - t4).count();
    double bucketed_sv_s = std::chrono::duration<double>(t6 - t5).count();
    double total_ops = double(reps) * double(domains.size());

    std::printf("%-38s %10.3f s   %14.0f ops/s\n", "ankerl hash_map (shipped):", hash_s, total_ops / hash_s);
    std::printf("%-38s %10.3f s   %14.0f ops/s\n", "trie, vector<pair> children:", trie_s, total_ops / trie_s);
    std::printf("%-38s %10.3f s   %14.0f ops/s\n", "trie, ankerl dense_map children:", trie_dense_s, total_ops / trie_dense_s);
    std::printf("%-38s %10.3f s   %14.0f ops/s\n", "bucketed map<tld,{set,depth,len}>:", bucketed_s, total_ops / bucketed_s);
    std::printf("%-38s %10.3f s   %14.0f ops/s\n", "bucketed, owned + string_view keys:", bucketed_owned_s, total_ops / bucketed_owned_s);
    std::printf("%-38s %10.3f s   %14.0f ops/s\n", "bucketed, string_view STORAGE (v3):", bucketed_sv_s, total_ops / bucketed_sv_s);
    std::printf("\nv3 (sv storage) vs owned(std::string keys): %.2fx\n", bucketed_owned_s / bucketed_sv_s);
    std::printf("v3 (sv storage) vs shipped hash_map:         %.2fx  (>1 means v3 is faster)\n", hash_s / bucketed_sv_s);

    return sink == 0xdeadbeef ? 1 : 0;  // keep sink live, never true
}
