#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#ifndef C10_TEST_CORE_MACROS_MACROS_H_

#ifdef _WIN32
#define DISABLED_ON_WINDOWS(x) DISABLED_##x
#else
#define DISABLED_ON_WINDOWS(x) x
#endif

#endif // C10_MACROS_MACROS_H_

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
