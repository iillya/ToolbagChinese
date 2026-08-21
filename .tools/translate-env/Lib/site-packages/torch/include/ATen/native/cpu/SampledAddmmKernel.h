#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/core/Tensor.h>
#include <ATen/native/DispatchStub.h>

namespace at::native {

using sampled_addmm_sparse_csr_fn = void(*)(const Tensor&, const Tensor&, const Scalar&, const Scalar&, const Tensor&);

DECLARE_DISPATCH(sampled_addmm_sparse_csr_fn, sampled_addmm_sparse_csr_stub)

} // at::native

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
