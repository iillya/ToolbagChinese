#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#include <ATen/Context.h>

namespace at {

/// Returns a detailed string describing the configuration PyTorch.
TORCH_API std::string show_config();

TORCH_API std::string get_mkl_version();

TORCH_API std::string get_mkldnn_version();

TORCH_API std::string get_openmp_version();

TORCH_API std::string get_cxx_flags();

TORCH_API std::string get_cpu_capability();

} // namespace at

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
