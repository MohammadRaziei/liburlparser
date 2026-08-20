// -----------------------------------------------------------------------------
// This file is AUTO-GENERATED. Do not edit manually.
// Generated for package: liburlparser
//
// Source file:
// @INPUT@
//
// Any changes should be made to the original data file and then
// regenerated via CMake (this happens automatically on configure).
// -----------------------------------------------------------------------------

#ifndef @INCLUDE_GUARD@
#define @INCLUDE_GUARD@
#pragma once

#include <string>

namespace @NAMESPACE@ {
    // NOTE: built from one string literal PER LINE ("a" "b" -> "ab"), not a
    // single R"tag(...)tag" raw string. A single raw/plain literal this large
    // (300+ KB) exceeds MSVC's per-literal length limit; adjacent literal
    // concatenation has no such limit and is portable to every compiler.
    inline const std::string @VAR@ =
@CONTENT@
    ;
} // namespace @NAMESPACE@

#endif // @INCLUDE_GUARD@
