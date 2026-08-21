#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit::onnx {

namespace ONNXScopeName {

std::string createFullScopeName(
    const std::string& class_name,
    const std::string& variable_name);
std::string variableName(const torch::jit::ScopePtr& scope);
std::string variableNameFromRoot(
    const torch::jit::ScopePtr& scope,
    const std::string& layer_separator);
std::string className(const torch::jit::ScopePtr& scope);
std::string classNameFromRoot(
    const torch::jit::ScopePtr& scope,
    const std::string& layer_separator);
bool isCompatibleScope(const torch::jit::ScopePtr& scope);

} // namespace ONNXScopeName

TORCH_API void AssignScopedNamesForNodeAndValue(std::shared_ptr<Graph>& graph);

} // namespace torch::jit::onnx

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
