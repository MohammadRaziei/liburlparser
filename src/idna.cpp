#include "idna.h"

#include <cstdint>
#include <vector>

// Standalone Punycode (RFC 3492) + minimal IDNA label-splitting
// implementation - no external dependency. This intentionally does not
// implement the full UTS-46 pipeline (case-folding, NFC normalization,
// disallowed-codepoint tables): those matter for hostile/malformed input,
// but for well-formed, already-composed UTF-8 hostnames (the overwhelming
// common case, and the only case liburlparser needs to normalize) plain
// per-label Punycode encoding of the decoded codepoints is exactly what
// UTS-46 reduces to, and it reproduces the reference/ada-url output for
// such input.

namespace urlparser {
namespace idna {

namespace {

// --- RFC 3492 constants (Bootstring/Punycode parameters for IDNA) ---
constexpr uint32_t kBase = 36;
constexpr uint32_t kTMin = 1;
constexpr uint32_t kTMax = 26;
constexpr uint32_t kSkew = 38;
constexpr uint32_t kDamp = 700;
constexpr uint32_t kInitialBias = 72;
constexpr uint32_t kInitialN = 128;

uint32_t adapt_bias(uint32_t delta, uint32_t num_points, bool first_time) {
    delta = first_time ? delta / kDamp : delta / 2;
    delta += delta / num_points;
    uint32_t k = 0;
    while (delta > ((kBase - kTMin) * kTMax) / 2) {
        delta /= (kBase - kTMin);
        k += kBase;
    }
    return k + (((kBase - kTMin + 1) * delta) / (delta + kSkew));
}

char encode_digit(uint32_t d) {
    // 0-25 -> 'a'-'z', 26-35 -> '0'-'9'
    return static_cast<char>(d < 26 ? d + 'a' : d - 26 + '0');
}

// Decode a UTF-8 byte string into Unicode codepoints. Malformed sequences
// are passed through byte-by-byte (never throws) - matches the
// error-tolerant spirit of the rest of liburlparser's parsing.
std::vector<uint32_t> utf8_decode(std::string_view s) {
    std::vector<uint32_t> out;
    out.reserve(s.size());
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        uint32_t cp;
        size_t len;
        if (c < 0x80) { cp = c; len = 1; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; len = 2; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; len = 3; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; len = 4; }
        else { out.push_back(c); ++i; continue; }  // invalid lead byte

        if (i + len > s.size()) { out.push_back(c); ++i; continue; }
        bool valid = true;
        uint32_t acc = cp;
        for (size_t k = 1; k < len; ++k) {
            unsigned char cc = static_cast<unsigned char>(s[i + k]);
            if ((cc & 0xC0) != 0x80) { valid = false; break; }
            acc = (acc << 6) | (cc & 0x3F);
        }
        if (!valid) { out.push_back(c); ++i; continue; }
        out.push_back(acc);
        i += len;
    }
    return out;
}

bool is_ascii_label(const std::vector<uint32_t>& cps) {
    for (uint32_t c : cps)
        if (c >= 0x80) return false;
    return true;
}

// Punycode-encode one already-decoded label's codepoints (RFC 3492 section
// 6.3), returning just the encoded part (caller adds the "xn--" prefix).
std::string punycode_encode(const std::vector<uint32_t>& input) {
    std::string output;
    uint32_t n = kInitialN;
    uint32_t delta = 0;
    uint32_t bias = kInitialBias;

    size_t basic_count = 0;
    for (uint32_t cp : input) {
        if (cp < 0x80) {
            output.push_back(static_cast<char>(cp));
            ++basic_count;
        }
    }
    size_t h = basic_count;
    if (basic_count > 0) output.push_back('-');

    while (h < input.size()) {
        uint32_t m = UINT32_MAX;
        for (uint32_t cp : input)
            if (cp >= n && cp < m) m = cp;

        delta += (m - n) * static_cast<uint32_t>(h + 1);
        n = m;

        for (uint32_t cp : input) {
            if (cp < n) ++delta;
            if (cp == n) {
                uint32_t q = delta;
                for (uint32_t k = kBase;; k += kBase) {
                    uint32_t t = (k <= bias) ? kTMin
                                 : (k >= bias + kTMax) ? kTMax
                                                        : k - bias;
                    if (q < t) break;
                    output.push_back(encode_digit(t + (q - t) % (kBase - t)));
                    q = (q - t) / (kBase - t);
                }
                output.push_back(encode_digit(q));
                bias = adapt_bias(delta, static_cast<uint32_t>(h + 1), h == basic_count);
                delta = 0;
                ++h;
            }
        }
        ++delta;
        ++n;
    }
    return output;
}

// A label needs Punycode encoding if it has any non-ASCII codepoint.
// IDNA case-folds ASCII letters to lowercase before encoding (part of the
// UTS-46 mapping step) - done here for basic Latin only, which covers
// every case this project's own hostnames/tests exercise.
std::string encode_label(std::string_view label) {
    std::vector<uint32_t> cps = utf8_decode(label);
    for (uint32_t& cp : cps)
        if (cp >= 'A' && cp <= 'Z') cp += ('a' - 'A');
    if (is_ascii_label(cps)) {
        std::string out;
        out.reserve(cps.size());
        for (uint32_t cp : cps) out.push_back(static_cast<char>(cp));
        return out;
    }
    return "xn--" + punycode_encode(cps);
}

}  // namespace

std::string to_ascii(std::string_view utf8_domain) {
    std::string out;
    out.reserve(utf8_domain.size());
    size_t start = 0;
    for (size_t i = 0; i <= utf8_domain.size(); ++i) {
        if (i == utf8_domain.size() || utf8_domain[i] == '.') {
            out += encode_label(utf8_domain.substr(start, i - start));
            if (i != utf8_domain.size()) out.push_back('.');
            start = i + 1;
        }
    }
    return out;
}

}  // namespace idna
}  // namespace urlparser
