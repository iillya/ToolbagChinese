// ============================================================================
//  Toolbag 5 Chinese Localizer - Self-contained one-click installer (EXE)
// ============================================================================
//  Everything needed (install.ps1 + dist files: dictionaries, DLL, launcher,
//  font) is appended to this EXE as embedded data. On launch it:
//     1. shows an install / uninstall menu
//     2. extracts the embedded files to a temp folder
//     3. runs scripts\install.ps1 (or with -Uninstall) from that folder
//     4. cleans up the temp folder
//  So the end user only needs this single EXE - nothing else beside it.
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
// Temp-folder helpers
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

} // namespace

int main() {
    std::vector<EmbeddedFile> files;
    if (!ReadEmbeddedFiles(files)) {
        MessageBoxA(nullptr,
                    "此 EXE 缺少嵌入的安装数据，请使用完整的单文件安装版。",
                    "八猴 Toolbag 5 汉化", MB_OK | MB_ICONERROR);
        return 1;
    }

    printf("============================================\n");
    printf("   八猴 Toolbag 5 汉化插件（单文件安装）\n");
    printf("============================================\n");
    printf("   [1] 安装汉化\n");
    printf("   [2] 卸载汉化（还原原版）\n");
    printf("============================================\n");
    printf("  请输入选择（1 或 2）: ");

    char choice[16] = {};
    if (!fgets(choice, sizeof choice, stdin)) choice[0] = '1';
    const bool uninstall = (choice[0] == '2');

    const std::string folder = MakeTempFolder();
    if (!ExtractAll(files, folder)) {
        MessageBoxA(nullptr, "解压安装数据失败。", "八猴 Toolbag 5 汉化", MB_OK | MB_ICONERROR);
        CleanupTempFolder(folder);
        return 2;
    }

    const std::string ps1 = folder + "scripts\\install.ps1";
    std::string command =
        "powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" + ps1 + "\"";
    if (uninstall) command += " -Uninstall";

    STARTUPINFOA startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};

    int exitCode = 3;
    if (CreateProcessA(nullptr, &command[0], nullptr, nullptr, TRUE, 0,
                       nullptr, nullptr, &startupInfo, &processInfo)) {
        WaitForSingleObject(processInfo.hProcess, INFINITE);
        DWORD code = 0;
        GetExitCodeProcess(processInfo.hProcess, &code);
        exitCode = (int)code;
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
    } else {
        MessageBoxA(nullptr, "无法启动 PowerShell，请确认系统已安装。",
                    "八猴 Toolbag 5 汉化", MB_OK | MB_ICONERROR);
    }

    CleanupTempFolder(folder);
    return exitCode;
}
