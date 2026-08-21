#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/core/impl/PyInterpreterHooks.h>

namespace torch::detail {

// Concrete implementation of PyInterpreterHooks
class PyInterpreterHooks : public c10::impl::PyInterpreterHooksInterface {
 public:
  PyInterpreterHooks();

  c10::impl::PyInterpreter* getPyInterpreter() const override;
};

} // namespace torch::detail

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
