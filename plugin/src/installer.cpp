// ============================================================================
//  Toolbag 5 Chinese Localizer - Self-contained one-click installer (GUI EXE)
// ============================================================================
//  A GUI (no console) self-contained installer:
//     1. shows a MessageBox to choose 安装 / 卸载 / 取消
//     2. extracts embedded files to a temp folder
//     3. silently runs scripts\install.ps1 (or with -Uninstall)
//     4. cleans up the temp folder
//     5. shows a result MessageBox
//  All UI is Unicode (MessageBoxW) so Chinese never turns into garbled text.
//
//  Embedded payload format (appended after the PE, read via file I/O):
//      [ file data... ][ entries... ][ manifestSize u32 ][ count u32 ][ magic u64 ]
//  Each entry: [ nameLen u32 ][ name bytes ][ offset u64 ][ size u64 ]
// ============================================================================

#include <Windows.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>

namespace {

constexpr uint64_t kPayloadMagic = 0x314D484254ull; // "TBHM1"
const wchar_t* kAppTitle = L"八猴 Toolbag 5 汉化插件";

struct EmbeddedFile {
    std::string name;
    uint64_t offset = 0;
    uint64_t size = 0;
};

// ---------------------------------------------------------------------------
// Embedded-payload reader
// ---------------------------------------------------------------------------
bool ReadEmbeddedFiles(std::vector<EmbeddedFile>& files) {
    char self[MAX_PATH];
    GetModuleFileNameA(nullptr, self, MAX_PATH);

    FILE* file = nullptr;
    if (fopen_s(&file, self, "rb") || !file) return false;

    fseek(file, 0, SEEK_END);
    const long fileSize = ftell(file);

    uint64_t magic = 0;
    uint32_t count = 0;
    uint32_t manifestSize = 0;
    fseek(file, fileSize - 8,  SEEK_SET); fread(&magic, 8, 1, file);
    fseek(file, fileSize - 12, SEEK_SET); fread(&count, 4, 1, file);
    fseek(file, fileSize - 16, SEEK_SET); fread(&manifestSize, 4, 1, file);
    if (magic != kPayloadMagic || count == 0) { fclose(file); return false; }

    const long entriesStart = fileSize - 16 - static_cast<long>(manifestSize);
    fseek(file, entriesStart, SEEK_SET);

    for (uint32_t i = 0; i < count; i++) {
        uint32_t nameLen = 0;
        if (fread(&nameLen, 4, 1, file) != 1) break;
        std::vector<char> name(nameLen + 1);
        if (fread(name.data(), 1, nameLen, file) != nameLen) break;
        name[nameLen] = 0;
        EmbeddedFile f;
        f.name = name.data();
        fread(&f.offset, 8, 1, file);
        fread(&f.size, 8, 1, file);
        files.push_back(std::move(f));
    }
    fclose(file);
    return !files.empty();
}

// ---------------------------------------------------------------------------
// Temp-folder helpers (ASCII paths: temp folder + known file names)
// ---------------------------------------------------------------------------
std::string MakeTempFolder() {
    char temp[MAX_PATH];
    GetTempPathA(MAX_PATH, temp);
    char folder[MAX_PATH];
    sprintf_s(folder, "%sToolbagChineseLocalizer_%lu\\", temp, GetCurrentProcessId());
    CreateDirectoryA(folder, nullptr);
    CreateDirectoryA((std::string(folder) + "scripts").c_str(), nullptr);
    CreateDirectoryA((std::string(folder) + "dist").c_str(), nullptr);
    return folder;
}

void CleanupTempFolder(const std::string& folder) {
    for (const auto* f : {"scripts\\install.ps1", "dist\\__main__.py", "dist\\dictionary.txt",
                          "dist\\dictionary_assets.txt", "dist\\ToolbagChineseHook.dll",
                          "dist\\ToolbagChineseLauncher.exe", "dist\\deng_ui.slug"}) {
        DeleteFileA((folder + f).c_str());
    }
    RemoveDirectoryA((folder + "scripts").c_str());
    RemoveDirectoryA((folder + "dist").c_str());
    RemoveDirectoryA(folder.c_str());
}

bool ExtractAll(const std::vector<EmbeddedFile>& files, const std::string& folder) {
    char self[MAX_PATH];
    GetModuleFileNameA(nullptr, self, MAX_PATH);
    FILE* in = nullptr;
    if (fopen_s(&in, self, "rb") || !in) return false;

    bool ok = true;
    for (const auto& f : files) {
        const std::string dest = folder + f.name;
        FILE* out = nullptr;
        if (fopen_s(&out, dest.c_str(), "wb") || !out) { ok = false; break; }
        fseek(in, static_cast<long>(f.offset), SEEK_SET);
        std::vector<unsigned char> buf(1 << 20);
        uint64_t remaining = f.size;
        while (remaining > 0) {
            const size_t chunk = remaining > buf.size() ? buf.size() : (size_t)remaining;
            if (fread(buf.data(), 1, chunk, in) != chunk) { ok = false; break; }
            fwrite(buf.data(), 1, chunk, out);
            remaining -= chunk;
        }
        fclose(out);
        if (!ok) break;
    }
    fclose(in);
    return ok;
}

// Convert an ASCII path to a wide string.
std::wstring ToWide(const std::string& s) {
    std::wstring w;
    for (char c : s) w += static_cast<wchar_t>(static_cast<unsigned char>(c));
    return w;
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    std::vector<EmbeddedFile> files;
    if (!ReadEmbeddedFiles(files)) {
        MessageBoxW(nullptr, L"此 EXE 缺少嵌入的安装数据，请使用完整的单文件安装版。",
                    kAppTitle, MB_OK | MB_ICONERROR);
        return 1;
    }

    // GUI menu: 安装 / 卸载 / 取消
    const int choice = MessageBoxW(
        nullptr,
        L"八猴 Toolbag 5 汉化插件\n\n"
        L"选择要执行的操作：\n\n"
        L"  是(Y)  ->  安装汉化\n"
        L"  否(N)  ->  卸载汉化（还原原版）\n"
        L"  取消    ->  退出",
        kAppTitle, MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1);
    if (choice == IDCANCEL) return 0;
    const bool uninstall = (choice == IDNO);

    const std::string folder = MakeTempFolder();
    if (!ExtractAll(files, folder)) {
        MessageBoxW(nullptr, L"解压安装数据失败，请重试。", kAppTitle, MB_OK | MB_ICONERROR);
        CleanupTempFolder(folder);
        return 2;
    }

    const std::wstring ps1 = ToWide(folder) + L"scripts\\install.ps1";
    std::wstring cmd =
        L"powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" + ps1 + L"\" -Quiet";
    if (uninstall) cmd += L" -Uninstall";

    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};

    int exitCode = 3;
    if (CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                       nullptr, nullptr, &startupInfo, &processInfo)) {
        WaitForSingleObject(processInfo.hProcess, INFINITE);
        DWORD code = 0;
        GetExitCodeProcess(processInfo.hProcess, &code);
        exitCode = (int)code;
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
    } else {
        MessageBoxW(nullptr, L"无法启动 PowerShell，请确认系统已安装。",
                    kAppTitle, MB_OK | MB_ICONERROR);
        CleanupTempFolder(folder);
        return 3;
    }

    CleanupTempFolder(folder);

    // Result
    if (exitCode == 0) {
        MessageBoxW(nullptr,
                    uninstall
                        ? L"汉化已卸载完成！\n\n插件已删除、原版字体已还原、日志已清理，Toolbag 恢复正常。"
                        : L"汉化安装完成！\n\n请通过插件目录中的 ToolbagChineseLauncher.exe 启动 Toolbag。",
                    kAppTitle, MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    MessageBoxW(nullptr,
                uninstall
                    ? L"卸载未成功完成，请查看上方错误信息后重试。"
                    : L"安装未成功完成，请查看上方错误信息后重试。",
                kAppTitle, MB_OK | MB_ICONERROR);
    return exitCode;
}
