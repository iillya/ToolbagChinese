#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <memory>

#include <torch/csrc/jit/ir/ir.h>
#include <optional>

namespace torch::jit {

const int ONNX_OPSET_9 = 9;
const int ONNX_OPSET_10 = 10;
const int ONNX_OPSET_11 = 11;
const int ONNX_OPSET_12 = 12;
const int ONNX_OPSET_13 = 13;
const int ONNX_OPSET_14 = 14;

namespace onnx_constant_fold {

at::Tensor IntToTensor(int64_t value);

std::optional<at::Tensor> runTorchBackendForOnnx(
    const Node* node,
    std::vector<at::Tensor>& inputTensorValues,
    int opset_version);
} // namespace onnx_constant_fold

void ConstantFoldONNX(
    std::shared_ptr<Graph>& g,
    std::map<std::string, IValue>& paramDict,
    int opset_version);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
