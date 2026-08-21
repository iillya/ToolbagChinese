#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/optim/adagrad.h>
#include <torch/optim/adam.h>
#include <torch/optim/adamw.h>
#include <torch/optim/lbfgs.h>
#include <torch/optim/optimizer.h>
#include <torch/optim/rmsprop.h>
#include <torch/optim/sgd.h>

#include <torch/optim/schedulers/lr_scheduler.h>
#include <torch/optim/schedulers/reduce_on_plateau_scheduler.h>
#include <torch/optim/schedulers/step_lr.h>

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
