#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/tensorexpr/operators/conv2d.h>
#include <torch/csrc/jit/tensorexpr/operators/matmul.h>
#include <torch/csrc/jit/tensorexpr/operators/misc.h>
#include <torch/csrc/jit/tensorexpr/operators/norm.h>
#include <torch/csrc/jit/tensorexpr/operators/pointwise.h>
#include <torch/csrc/jit/tensorexpr/operators/quantization.h>
#include <torch/csrc/jit/tensorexpr/operators/reduction.h>
#include <torch/csrc/jit/tensorexpr/operators/softmax.h>

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
