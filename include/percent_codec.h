#ifndef URLPARSER_PERCENT_CODEC_H
#define URLPARSER_PERCENT_CODEC_H

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace urlparser {

/**
 * @brief Percent-encoding (RFC 3986 "quote") / percent-decoding ("unquote").
 *
 * liburlparser exposes path()/query()/params() as the *raw* (still
 * percent-encoded) substrings of the URL - same as the WHATWG URL standard
 * and every URL library that follows it (including ada). None of them
 * decode automatically, on purpose: it can be lossy/ambiguous to do so
 * implicitly. But unlike Python's urllib.parse (quote/unquote) or ada's
 * internal ada::unicode::percent_encode/percent_decode, liburlparser did not
 * expose *any* public helper to do this decoding explicitly - so callers had
 * to reach for a second library just to read "café" out of "caf%C3%A9".
 * This header adds that missing pair of primitives, implemented from
 * scratch (no external dependency): a 256-bit membership bitmap gives O(1)
 * "does this byte need encoding" checks, and both functions do a single
 * allocation sized to a conservative upper bound.
 */
namespace percent_codec {

/// A 256-bit set of bytes, used to mark which bytes are left untouched by
/// percent_encode(). Bit `i` set means "byte value `i` is safe, do not
/// encode it".
using character_set = std::array<uint64_t, 4>;

constexpr bool contains(const character_set& set, unsigned char c) noexcept {
    return (set[c >> 6] >> (c & 63)) & 1ULL;
}

/// RFC 3986 "unreserved" characters: A-Z a-z 0-9 - _ . ~
/// (the default safe set for percent_encode(), matching Python's
/// urllib.parse.quote() default minus its '/' addition - see path_safe()).
character_set unreserved_set();

/// unreserved_set() plus '/' - the common default when encoding a full path
/// component (mirrors Python's urllib.parse.quote(..., safe="/")).
character_set path_safe_set();

/**
 * @brief Percent-decode ("unquote") a string: every %XX triplet (two hex
 * digits) becomes the corresponding byte; anything that isn't a valid %XX
 * triplet (e.g. a trailing "%", or "%" followed by non-hex digits) is left
 * untouched, byte-for-byte, matching Python's urllib.parse.unquote()
 * behaviour for malformed input rather than throwing.
 */
std::string decode(std::string_view input);

/**
 * @brief Percent-encode ("quote") a string: every byte not in `safe` is
 * replaced by %XX (uppercase hex). Defaults to unreserved_set() (RFC 3986
 * unreserved characters only) when no character_set is given.
 */
std::string encode(std::string_view input,
                    const character_set& safe = unreserved_set());

}  // namespace percent_codec
}  // namespace urlparser

#endif  // URLPARSER_PERCENT_CODEC_H
