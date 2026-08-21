#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#if !defined(_MSC_VER) && __cplusplus < 201703L
#error C++17 or later compatible compiler is required to use PyTorch.
#endif

#include <torch/autograd.h>
#include <torch/cuda.h>
#include <torch/data.h>
#include <torch/enum.h>
#include <torch/fft.h>
#include <torch/jit.h>
#include <torch/mps.h>
#include <torch/nested.h>
#include <torch/nn.h>
#include <torch/optim.h>
#include <torch/print.h>
#include <torch/serialize.h>
#include <torch/sparse.h>
#include <torch/special.h>
#include <torch/types.h>
#include <torch/utils.h>
#include <torch/version.h>
#include <torch/xpu.h>

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
