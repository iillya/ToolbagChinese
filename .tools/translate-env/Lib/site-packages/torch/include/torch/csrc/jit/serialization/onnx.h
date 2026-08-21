#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

C10_DIAGNOSTIC_PUSH_AND_IGNORED_IF_DEFINED(
    "-Winconsistent-missing-destructor-override")
C10_DIAGNOSTIC_PUSH_AND_IGNORED_IF_DEFINED("-Wsuggest-override")
C10_DIAGNOSTIC_PUSH_AND_IGNORED_IF_DEFINED(
    "-Wdeprecated-dynamic-exception-spec")
#include <onnx/onnx_pb.h>
C10_DIAGNOSTIC_POP()
C10_DIAGNOSTIC_POP()
C10_DIAGNOSTIC_POP()
#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

TORCH_API std::string prettyPrint(const ::ONNX_NAMESPACE::ModelProto& model);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
