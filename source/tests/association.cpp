#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../inno/support.cpp"
#include <cassert>
#include <iostream>

namespace {
bool FailSecondWrite(const wchar_t* phase, unsigned index) {
    return wcscmp(phase, L"before-apply") != 0 || index != 1;
}
}

int main() {
    // Fake registry hives live entirely under this isolated test key. No real
    // Software\Classes or file association is written by this test.
    using namespace ToolbagProxy;
    const std::wstring testPath = L"Software\\ToolbagChineseAudit_" + std::to_wstring(GetCurrentProcessId());
    {
        Key sandbox, user, machine;
        assert(RegCreateKeyExW(HKEY_CURRENT_USER, testPath.c_str(), 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &sandbox.value, nullptr) == ERROR_SUCCESS);
        assert(RegCreateKeyExW(sandbox.value, L"user", 0, nullptr, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, nullptr, &user.value, nullptr) == ERROR_SUCCESS);
        assert(RegCreateKeyExW(sandbox.value, L"machine", 0, nullptr, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, nullptr, &machine.value, nullptr) == ERROR_SUCCESS);
        const std::wstring root = L"C:\\Audit Toolbag";
        const auto original = stringValue(L"\"" + root + L"\\toolbag.exe\" --open \"%1\" --keep", REG_EXPAND_SZ);
        for (const auto* handler : kKnownHandlers)
            assert(write(machine.value, L"Software\\Classes\\" + std::wstring(handler) + L"\\shell\\open\\command", nullptr, original));
        assert(write(machine.value, L"Software\\Classes\\.tbscene", nullptr, stringValue(L"MarmosetToolbag5.Scene")));
        std::wstring error;
        assert(install(user.value, machine.value, root, error));
        Value proxy, unchanged;
        assert(read(user.value, path(0), nullptr, proxy));
        std::wstring command;
        assert(text(proxy, command) && proxy.type == REG_EXPAND_SZ);
        assert(command == L"\"" + root + L"\\ChineseLauncher\\ToolbagChineseLauncher.exe\" --open \"%1\" --keep");
        assert(read(machine.value, path(0), nullptr, unchanged) && unchanged == original);
        const auto external = stringValue(L"\"C:\\Other\\editor.exe\" \"%1\"");
        assert(write(user.value, path(0), nullptr, external));
        assert(uninstall(user.value, root, error));
        assert(read(user.value, path(0), nullptr, unchanged) && unchanged == external);
        assert(read(user.value, path(1), nullptr, unchanged) && !unchanged.present);
        assert(write(user.value, path(0), nullptr, {}));
        assert(!install(user.value, machine.value, root, error, FailSecondWrite));
        assert(read(user.value, path(0), nullptr, unchanged) && !unchanged.present);
        assert(read(user.value, path(1), nullptr, unchanged) && !unchanged.present);
        Value rejected;
        assert(!makeProxy(stringValue(L"\"C:\\Unknown\\app.exe\" \"%1\""),
            root + L"\\toolbag.exe", root + L"\\ChineseLauncher\\ToolbagChineseLauncher.exe", rejected));
    }
    assert(RegDeleteTreeW(HKEY_CURRENT_USER, testPath.c_str()) == ERROR_SUCCESS);
    std::cout << "PASS: isolated association proxy preserves arguments/types/official commands/external edits and rolls back failed writes\n";
}
