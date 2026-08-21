#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/TensorIterator.h>
#include <ATen/native/DispatchStub.h>

namespace at::native {

using renorm_scale_factor_fn = void (*) (TensorIteratorBase& iter, double maxnorm);
DECLARE_DISPATCH(renorm_scale_factor_fn, renorm_scale_factor_stub)

enum class BatchNormBackend {
  Native,
  Cudnn,
  Miopen,
};

TORCH_API BatchNormBackend _select_batch_norm_backend(const Tensor& input, const Tensor& weight, const Tensor& bias, const Tensor& running_mean, const Tensor& running_var, bool training, double eps);

}  // namespace at::native

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
