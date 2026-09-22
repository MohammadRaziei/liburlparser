#include "percent_codec.h"

namespace urlparser {
namespace percent_codec {

namespace {

constexpr char hex_digits[] = "0123456789ABCDEF";

// -1 for non-hex bytes; lookup table avoids branching per character.
constexpr std::array<int8_t, 256> make_hex_value_table() {
    std::array<int8_t, 256> table{};
    for (auto& v : table) v = -1;
    for (int c = '0'; c <= '9'; ++c) table[c] = static_cast<int8_t>(c - '0');
    for (int c = 'a'; c <= 'f'; ++c) table[c] = static_cast<int8_t>(c - 'a' + 10);
    for (int c = 'A'; c <= 'F'; ++c) table[c] = static_cast<int8_t>(c - 'A' + 10);
    return table;
}

constexpr std::array<int8_t, 256> hex_value = make_hex_value_table();

constexpr character_set make_unreserved_set() {
    character_set set{};
    auto mark = [&](unsigned char c) { set[c >> 6] |= (1ULL << (c & 63)); };
    for (unsigned char c = 'A'; c <= 'Z'; ++c) mark(c);
    for (unsigned char c = 'a'; c <= 'z'; ++c) mark(c);
    for (unsigned char c = '0'; c <= '9'; ++c) mark(c);
    mark('-');
    mark('_');
    mark('.');
    mark('~');
    return set;
}

}  // namespace

character_set unreserved_set() {
    static constexpr character_set set = make_unreserved_set();
    return set;
}

character_set path_safe_set() {
    character_set set = unreserved_set();
    set['/' >> 6] |= (1ULL << ('/' & 63));
    return set;
}

std::string decode(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    const size_t n = input.size();
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        if (c == '%' && i + 2 < n) {
            int8_t hi = hex_value[static_cast<unsigned char>(input[i + 1])];
            int8_t lo = hex_value[static_cast<unsigned char>(input[i + 2])];
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(static_cast<char>(c));
    }
    return out;
}

std::string encode(std::string_view input, const character_set& safe) {
    std::string out;
    out.reserve(input.size());  // grown on demand for the escaped bytes
    for (unsigned char c : input) {
        if (contains(safe, c)) {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(hex_digits[c >> 4]);
            out.push_back(hex_digits[c & 0x0F]);
        }
    }
    return out;
}

}  // namespace percent_codec
}  // namespace urlparser
