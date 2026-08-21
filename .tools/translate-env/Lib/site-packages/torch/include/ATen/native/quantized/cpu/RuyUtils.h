#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#ifdef USE_RUY_QMATMUL

#include <ruy/ruy.h>

namespace at::native::ruy_utils {

ruy::Context* get_ruy_context();

void quantize_multiplier(double scale,
                         int* multiplier_fixedpoint,
                         int* multiplier_exponent);

} // namespace at::native::ruy_utils

#endif // USE_RUY_QMATMUL

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
