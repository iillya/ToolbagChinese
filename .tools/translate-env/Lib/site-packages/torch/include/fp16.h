#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#ifndef FP16_H
#define FP16_H

#include <fp16/fp16.h>

#if defined(PSIMD_H)
#include <fp16/psimd.h>
#endif

#endif /* FP16_H */

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
