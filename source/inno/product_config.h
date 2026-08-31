#pragma once
#include <vector>
#include <string>
inline constexpr wchar_t kProductId[] = L"ToolbagChinese";
inline constexpr wchar_t kHostPattern[] = L"toolbag.exe";
inline constexpr wchar_t kLauncher[] = L"ToolbagChineseLauncher.exe";
inline const std::vector<const wchar_t*> kKnownHandlers = {L"MarmosetToolbag5.Scene",L"MarmosetToolbag.Scene",L"Applications\\toolbag.exe"};
inline const std::vector<const wchar_t*> kExtensions = {L".tbscene"};
inline const std::vector<const wchar_t*> kLegacyHandlers = {L"ChineseLocalizer.tbscene"};
inline const std::vector<const wchar_t*> kQtModules = {};
inline const std::vector<std::vector<DWORD>> kQtVersions = {};
