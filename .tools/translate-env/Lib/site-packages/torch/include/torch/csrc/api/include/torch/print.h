#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/macros/Export.h>

namespace torch {

/// Set whether to use scientific notation when printing tensors
///
/// When enabled (default), tensors with large or small values will use
/// scientific notation (e.g., 1.0000e+05). When disabled, they will use
/// fixed-point notation (e.g., 100000.0).
///
/// \param enabled Whether to enable scientific notation
TORCH_API void set_printoption_sci_mode(bool enabled);

} // namespace torch

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
