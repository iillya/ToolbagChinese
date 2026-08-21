#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/macros/Export.h>

#ifdef THP_BUILD_MAIN_LIB
#define TORCH_PYTHON_API C10_EXPORT
#else
#define TORCH_PYTHON_API C10_IMPORT
#endif

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
