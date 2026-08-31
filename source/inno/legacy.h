#pragma once
// Conservative import of prior installer records. Never delete an official class
// or UserChoice; legacy classes are retained with an official open command.
namespace ProductLegacy {
using namespace CascadeurProxy;
inline bool exactLauncher(const Value& value, const std::wstring& launcher) {
    std::wstring command;
    return text(value, command) && command == L"\"" + launcher + L"\" \"%1\"";
}
inline bool isLegacyHandler(const std::wstring& name) {
    for (const auto* legacy : kLegacyHandlers) if (_wcsicmp(legacy, name.c_str()) == 0) return true;
    return false;
}
// Recovery, not reconstruction of unknown historical values. Only the complete
// absence of the old journal is eligible; a damaged/partial journal still fails.
inline bool officialRecovery(HKEY user, HKEY machine, const std::wstring& official, Value& command) {
    Key backup;
    const auto status = RegOpenKeyExW(user, L"Software\\MarmosetChineseLocalizer\\FileAssociationBackup",
        0, KEY_READ, &backup.value);
    if (status != ERROR_FILE_NOT_FOUND && status != ERROR_PATH_NOT_FOUND) return false;
    Value directory, association;
    std::wstring folder, handler;
    if (!read(user, L"Software\\MarmosetChineseLocalizer", L"ToolbagDir", directory) ||
        !text(directory, folder) || _wcsicmp((folder + L"\\toolbag.exe").c_str(), official.c_str()) != 0 ||
        !regularFile(official) || !amd64(official) ||
        !read(machine, L"Software\\Classes\\.tbscene", nullptr, association) || !text(association, handler) ||
        (handler != L"MarmosetToolbag5.Scene" && handler != L"MarmosetToolbag.Scene")) return false;
    Value proxy;
    return read(machine, L"Software\\Classes\\" + handler + L"\\shell\\open\\command", nullptr, command) &&
        makeProxy(command, official, folder + L"\\ChineseLauncher\\ToolbagChineseLauncher.exe", proxy);
}

inline bool importOriginal(HKEY user, HKEY machine, const std::wstring& handler, const std::wstring& launcher,
                           const std::wstring& official, Value& original, std::wstring& error) {
    if (!exactLauncher(original, launcher)) return true;
    if (isLegacyHandler(handler)) {
        // Keep this legacy ProgID usable if Windows still references it.
        original = stringValue(L"\"" + official + L"\" \"%1\"");
        return true;
    }
    if (std::wstring(kProductId) == L"ToolbagChinese") {
        const std::wstring names[] = {L"MarmosetToolbag5.Scene", L"MarmosetToolbag.Scene", L"Applications\\toolbag.exe"};
        for (unsigned i = 0; i < 3; ++i) if (_wcsicmp(handler.c_str(), names[i].c_str()) == 0) {
            const auto prefix = L"Handler" + std::to_wstring(i);
            const auto backup = L"Software\\MarmosetChineseLocalizer\\FileAssociationBackup";
            DWORD saved = 0, hadRoot = 0, hadCommand = 0, hadDefault = 0;
            if (!dword(user, backup, (prefix + L"Saved").c_str(), saved) || saved != 1 ||
                !dword(user, backup, (prefix + L"HadRoot").c_str(), hadRoot) || hadRoot > 1 ||
                !dword(user, backup, (prefix + L"HadCommand").c_str(), hadCommand) || hadCommand > 1 ||
                !dword(user, backup, (prefix + L"HadDefault").c_str(), hadDefault) || hadDefault > 1 ||
                (!hadRoot && (hadCommand || hadDefault)) || (!hadCommand && hadDefault)) break;
            if (!hadDefault) { original = {}; return true; }
            Value old; std::wstring command;
            if (!read(user, backup, (prefix + L"Value").c_str(), old) || !text(old, command) || command.empty() || exactLauncher(old, launcher)) break;
            original = old;
            return true;
        }
        Value canonical;
        bool known = false;
        for (const auto& name : names) if (_wcsicmp(handler.c_str(), name.c_str()) == 0) known = true;
        if (known && officialRecovery(user, machine, official, canonical)) {
            Value machineCommand, proxy;
            if (!read(machine, L"Software\\Classes\\" + handler + L"\\shell\\open\\command", nullptr, machineCommand)) return false;
            if (machineCommand.present) {
                if (!makeProxy(machineCommand, official, launcher, proxy)) return false;
                original = {}; // Restore inheritance from the verified official command.
            } else original = canonical; // Existing orphan handler: point back to official.
            return true;
        }
    }
    error = L"检测到旧版接管命令，但无法验证其原始备份。请保留备份并检查后重试。";
    return false;
}

inline bool restoreToolbagCommands(HKEY user, HKEY machine, const std::wstring& root,
                                   std::wstring& error, bool validateOnly = false) {
    const auto launcher = root + L"\\ChineseLauncher\\" + kLauncher;
    const auto official = root + L"\\" + hostName(root);
    std::vector<std::pair<std::wstring, Value>> restores;
    for (const auto* name : {L"MarmosetToolbag5.Scene", L"MarmosetToolbag.Scene", L"Applications\\toolbag.exe"}) {
        const auto key = L"Software\\Classes\\" + std::wstring(name) + L"\\shell\\open\\command";
        Value current;
        if (!read(user, key, nullptr, current)) return false;
        if (!exactLauncher(current, launcher)) continue;
        if (!importOriginal(user, machine, name, launcher, official, current, error)) return false;
        restores.emplace_back(key, current);
    }
    if (validateOnly) return true;
    for (const auto& entry : restores) {
        Value current;
        if (!read(user, entry.first, nullptr, current)) return false;
        if (exactLauncher(current, launcher) && !write(user, entry.first, nullptr, entry.second)) return false;
    }
    return true;
}

inline bool restoreExtensions(HKEY user, const std::wstring& root, std::wstring& error, bool validateOnly = false,
                              HKEY machine = HKEY_LOCAL_MACHINE) {
    const auto launcher = root + L"\\ChineseLauncher\\" + kLauncher;
    const auto official = root + L"\\" + hostName(root);
    for (size_t i = 0; i < kExtensions.size(); ++i) {
        if (i >= kLegacyHandlers.size()) break;
        const auto extPath = L"Software\\Classes\\" + std::wstring(kExtensions[i]);
        const auto classPath = L"Software\\Classes\\" + std::wstring(kLegacyHandlers[i]) + L"\\shell\\open\\command";
        Value current, command;
        std::wstring id, commandText;
        if (!read(user, extPath, nullptr, current) || !read(user, classPath, nullptr, command)) return false;
        if (!text(current, id) || id != kLegacyHandlers[i]) continue;
        if (!text(command, commandText) || (!exactLauncher(command, launcher) &&
            commandText != L"\"" + official + L"\" \"%1\"")) continue;
        const std::wstring product(kProductId);
        const bool toolbag = product == L"ToolbagChinese";
        const auto backup = toolbag ? L"Software\\MarmosetChineseLocalizer\\FileAssociationBackup" :
            L"Software\\" + product + L"Localizer\\AssocBackup";
        const auto prefix = std::wstring(kExtensions[i]).substr(1);
        DWORD had = 0;
        if (!dword(user, backup, toolbag ? L"HadDefault" : (prefix + L"_had_default").c_str(), had) || had > 1) {
            Value canonical;
            if (!toolbag || !officialRecovery(user, machine, official, canonical)) {
                error = L"旧版关联备份无法验证，也未找到匹配当前软件的官方恢复依据。未修改关联。";
                return false;
            }
            had = 0; // Recovery to the verified machine-level .tbscene default.
        }
        if (product == L"MariChinese") {
            DWORD saved = 0;
            if (!dword(user, backup, (prefix + L"_saved").c_str(), saved) || saved != 1) return false;
        }
        Value original;
        std::wstring originalId;
        if (had && (!read(user, backup, toolbag ? L"DefaultValue" : (prefix + L"_default").c_str(), original) ||
            !text(original, originalId) || originalId.empty() || originalId == id)) return false;
        if (validateOnly) continue;
        if (!write(user, extPath, nullptr, original)) return false;
        // Keep the old backup and ProgID: UserChoice may still refer to the latter.
        if (exactLauncher(command, launcher) && !write(user, classPath, nullptr,
            stringValue(L"\"" + official + L"\" \"%1\""))) return false;
    }
    return true;
}
}
