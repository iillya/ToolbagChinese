#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
// This file is temporary until native_functions.yaml and derivatives.yaml are
// merged. Ideally this should all go into native_functions.yaml

#include <torch/csrc/Export.h>
#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

TORCH_API std::optional<std::shared_ptr<Graph>> GetDecomposition(
    const FunctionSchema& schema);

TORCH_API void RegisterDecomposition(
    const FunctionSchema& schema,
    std::shared_ptr<Graph> g);

TORCH_API void RunDecompositions(std::shared_ptr<Graph> g);

TORCH_API std::optional<GraphFunction*> GetDecompositionFunction(
    const FunctionSchema& schema);

// For invocation in C++, recommended is to assign to static local variable
TORCH_API Function* GetDecompositionExecutor(const char* schema_literal);

TORCH_API Function* GetDecompositionExecutor(const FunctionSchema& schema);

TORCH_API void run_jit_decomposition(
    const c10::OperatorHandle& op,
    torch::jit::Stack* stack);

TORCH_API bool has_jit_decomposition(const FunctionSchema& schema);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
