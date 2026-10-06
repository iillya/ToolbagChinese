// Product adaptation for the shared Inno support DLL. Included inside support.cpp.
#include "product_config.h"

std::wstring hostName(const std::wstring&) { return kHostPattern; }

bool productExtraPaths(const std::wstring& root) {
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
