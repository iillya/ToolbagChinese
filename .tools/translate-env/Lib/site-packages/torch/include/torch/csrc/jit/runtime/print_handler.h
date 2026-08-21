#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/Export.h>

#include <string>

namespace torch::jit {

using PrintHandler = void (*)(const std::string&);

TORCH_API PrintHandler getDefaultPrintHandler();
TORCH_API PrintHandler getPrintHandler();
TORCH_API void setPrintHandler(PrintHandler ph);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
