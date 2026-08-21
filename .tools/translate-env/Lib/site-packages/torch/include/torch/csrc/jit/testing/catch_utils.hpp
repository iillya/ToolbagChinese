#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#define CATCH_CONFIG_PREFIX_ALL
#include <catch.hpp>

// CATCH_REQUIRE_THROWS is not defined identically to REQUIRE_THROWS and causes
// warning; define our own version that doesn't warn.
#define _CATCH_REQUIRE_THROWS(...) \
  INTERNAL_CATCH_THROWS(           \
      "CATCH_REQUIRE_THROWS", Catch::ResultDisposition::Normal, __VA_ARGS__)

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
