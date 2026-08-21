#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/macros/Export.h>
#include <cstdint>

// A simple thread local enumeration, used to link forward and backward pass
// ops and is used by autograd and observers framework
namespace at::sequence_number {

TORCH_API uint64_t peek();
TORCH_API uint64_t get_and_increment();

} // namespace at::sequence_number

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
