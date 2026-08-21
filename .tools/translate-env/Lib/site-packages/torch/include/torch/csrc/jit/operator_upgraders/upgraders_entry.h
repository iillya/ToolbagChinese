#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <c10/macros/Export.h>
#include <torch/csrc/jit/ir/ir.h>
#include <string>
#include <unordered_map>

namespace torch::jit {

TORCH_API void populate_upgraders_graph_map();

TORCH_API std::unordered_map<std::string, std::shared_ptr<Graph>>
generate_upgraders_graph();

TORCH_API std::unordered_map<std::string, std::string> get_upgraders_entry_map();

std::shared_ptr<Graph> create_upgrader_graph(
    const std::string& upgrader_name,
    const std::string& upgrader_body);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
