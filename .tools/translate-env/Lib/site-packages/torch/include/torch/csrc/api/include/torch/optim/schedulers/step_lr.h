#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/optim/schedulers/lr_scheduler.h>

namespace torch::optim {

class TORCH_API StepLR : public LRScheduler {
 public:
  StepLR(
      torch::optim::Optimizer& optimizer,
      const unsigned step_size,
      const double gamma = 0.1);

 private:
  std::vector<double> get_lrs() override;

  const unsigned step_size_;
  const double gamma_;
};
} // namespace torch::optim

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
