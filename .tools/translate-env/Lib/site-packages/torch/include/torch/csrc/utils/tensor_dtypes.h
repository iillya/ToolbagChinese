#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/core/ScalarType.h>
#include <string>
#include <tuple>

namespace torch::utils {

std::pair<std::string, std::string> getDtypeNames(at::ScalarType scalarType);

void initializeDtypes();

} // namespace torch::utils

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
