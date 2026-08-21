#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/data/transforms/base.h>
#include <torch/data/transforms/collate.h>
#include <torch/data/transforms/lambda.h>
#include <torch/data/transforms/stack.h>
#include <torch/data/transforms/tensor.h>

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
