#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

using ModulePtr = c10::intrusive_ptr<c10::ivalue::Object>;

// Given a graph with of a method which first argument is %self, lower it to a
// graph where all attributes accesses are replaced with explicit inputs of the
// graph (rather than results of prim::GetAttr executed on %self).
//
// Returns a tuple (graph, parameters) where the last module.parameters.size()
// inputs to the graph are the trainable parameters used in this method. The
// remaining inputs are the true inputs to the function.
TORCH_API std::pair<std::shared_ptr<Graph>, std::vector<IValue>> LowerGraph(
    Graph& graph,
    const ModulePtr& self);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
