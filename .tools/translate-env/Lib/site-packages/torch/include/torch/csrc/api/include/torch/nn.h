#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/nn/cloneable.h>
#include <torch/nn/functional.h>
#include <torch/nn/init.h>
#include <torch/nn/module.h>
#include <torch/nn/modules.h>
#include <torch/nn/options.h>
#include <torch/nn/pimpl.h>
#include <torch/nn/utils.h>

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
