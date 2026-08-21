#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <torch/csrc/Export.h>
#include <torch/csrc/lazy/core/ir_metadata.h>
#include <optional>
#include <vector>

namespace torch::lazy {

std::optional<SourceLocation> TORCH_PYTHON_API GetPythonFrameTop();

std::vector<SourceLocation> TORCH_PYTHON_API GetPythonFrames();

} // namespace torch::lazy

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
