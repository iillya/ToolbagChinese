#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

namespace at::native::mps {

void scan_simple_mps_impl(
    const Tensor& self,
    const Tensor& output,
    int64_t dim,
    const std::string& op_name);

} // namespace at::native::mps

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
