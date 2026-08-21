#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#include <torch/csrc/jit/ir/ir.h>
#include <memory>

namespace torch::jit {
TORCH_API std::shared_ptr<Graph> TraceGraph(
    const std::shared_ptr<Graph>& graph,
    Stack& stack);
} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
