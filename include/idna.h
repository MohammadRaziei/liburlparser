#ifndef URLPARSER_IDNA_H
#define URLPARSER_IDNA_H

#include <string>
#include <string_view>

namespace urlparser {

/**
 * @brief IDNA (Internationalized Domain Names in Applications) support.
 *
 * liburlparser's own host parsing is ASCII-only per RFC 3986: a Unicode
 * hostname like "café.com" is accepted (it will not throw), but it is kept
 * verbatim rather than normalized, so it will not compare equal to its
 * canonical ASCII/Punycode form "xn--caf-dma.com" even though they are the
 * same domain. This namespace closes that gap with a standalone, dependency-
 * free implementation of per-label Punycode encoding (RFC 3492) - the same
 * public algorithm ada-url/ada and every other IDNA implementation use, but
 * implemented here from scratch rather than linked against ada. See
 * src/idna.cpp for the scope/limits of this implementation (it covers
 * well-formed, already-composed UTF-8 input - the common case - but does
 * not implement the full UTS-46 case-folding/normalization pipeline).
 */
namespace idna {

/**
 * @brief Convert a UTF-8 domain (possibly containing international
 * characters) to its canonical all-ASCII form (using Punycode for any
 * non-ASCII labels). ASCII-only input, including an already-punycoded
 * domain, is returned unchanged.
 * @param utf8_domain The UTF-8-encoded domain/hostname to normalize.
 * @return The ASCII/Punycode form, or an empty string if the input is not a
 * valid domain.
 */
std::string to_ascii(std::string_view utf8_domain);

}  // namespace idna
}  // namespace urlparser

#endif  // URLPARSER_IDNA_H
