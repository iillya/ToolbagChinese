#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#if defined(_WIN32)
#include <c10/util/Exception.h>
#include <c10/util/win32-headers.h>
#include <string>
#endif

namespace c10 {
#if defined(_WIN32)
C10_API std::wstring u8u16(const std::string& str);
C10_API std::string u16u8(const std::wstring& wstr);
#endif
} // namespace c10

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
