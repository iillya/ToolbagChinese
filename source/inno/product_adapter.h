// Product adaptation for the shared Inno support DLL. Included inside support.cpp.
#include "product_config.h"

std::wstring hostName(const std::wstring& root) {
    if (std::wstring(kHostPattern) != L"Mari*.exe") return kHostPattern;
    WIN32_FIND_DATAW data{};
    HANDLE search = FindFirstFileW((root + L"\\Mari*.exe").c_str(), &data);
    if (search == INVALID_HANDLE_VALUE) return {};
    std::wstring result;
    do {
        const std::wstring name(data.cFileName);
        if (!(data.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) &&
            name.size() > 8 && name[4] >= L'0' && name[4] <= L'9') {
            if (!result.empty()) { result.clear(); break; } // Ambiguous host: ask for a specific folder.
            result = name;
        }
    } while (FindNextFileW(search, &data));
    FindClose(search);
    return result;
}

bool productQt(const std::wstring& root) {
    if (kQtVersions.empty()) return true;
    unsigned major = 0;
    for (const auto& v : kQtVersions) {
        const auto core = root + L"\\Qt" + std::to_wstring(v[0]) + L"Core.dll";
        if (regularFile(core)) { if (major && major != v[0]) return false; major = v[0]; }
    }
    if (!major) return false;
    std::vector<DWORD> versionSeen;
    for (const auto* module : kQtModules) {
        const auto file = root + L"\\Qt" + std::to_wstring(major) + module + L".dll";
        if (!regularFile(file) || !amd64(file)) return false;
        DWORD ignored = 0;
        const DWORD length = GetFileVersionInfoSizeW(file.c_str(), &ignored);
        if (!length || length > 1024 * 1024) return false;
        std::vector<BYTE> bytes(length);
        if (!GetFileVersionInfoW(file.c_str(), 0, length, bytes.data())) return false;
        VS_FIXEDFILEINFO* info = nullptr; UINT size = 0;
        if (!VerQueryValueW(bytes.data(), L"\\", reinterpret_cast<void**>(&info), &size) ||
            size < sizeof(*info) || info->dwSignature != 0xfeef04bd) return false;
        const std::vector<DWORD> current{HIWORD(info->dwFileVersionMS), LOWORD(info->dwFileVersionMS), HIWORD(info->dwFileVersionLS)};
        bool compatible = false;
        for (const auto& baseline : kQtVersions) {
            if (current[0] != baseline[0]) continue;
            // Public Qt5 ABI users permit newer minor releases; Qt6 private adapters stay pinned.
            if (major == 5) compatible = current[1] > baseline[1] ||
                (current[1] == baseline[1] && current[2] >= baseline[2]);
            else compatible = current == baseline;
        }
        if (!compatible || (!versionSeen.empty() && versionSeen != current)) return false;
        versionSeen = current;
    }
    return true;
}

bool productExtraPaths(const std::wstring& root) {
    if (std::wstring(kProductId) != L"ToolbagChinese") return true;
    for (const auto* name : {L"data", L"data\\gui", L"data\\gui\\font"}) {
        const auto folder = root + L"\\" + name;
        const auto attr = GetFileAttributesW(folder.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY) || (attr & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
    }
    const auto fonts = root + L"\\data\\gui\\font\\";
    for (const auto* name : {L"notosans_chinese.slug", L"segoeui.slug", L"selawik.slug"}) {
        for (const auto& file : {fonts + name, fonts + name + L".ChineseLocalizer.backup"}) {
            const auto attr = GetFileAttributesW(file.c_str());
            if (attr != INVALID_FILE_ATTRIBUTES && !regularFile(file)) return false;
        }
    }
    return true;
}
