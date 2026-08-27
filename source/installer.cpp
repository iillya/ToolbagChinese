// ============================================================================
//  Toolbag 5 Chinese Localizer - Self-contained one-click installer (GUI EXE)
// ============================================================================
//  A GUI (no console) self-contained installer:
//     1. starts elevated and asks for one Toolbag directory
//     2. installs or uninstalls against that exact directory
//     3. extracts the embedded payload to a private temporary directory
//     4. runs scripts\install.ps1 and removes the temporary directory
//     5. reports the result
//  All UI is Unicode (MessageBoxW) so Chinese never turns into garbled text.
//
//  Embedded payload format (appended after the PE, read via file I/O):
//      [ file data... ][ entries... ][ manifestSize u32 ][ count u32 ][ magic u64 ]
//  Each entry: [ nameLen u32 ][ name bytes ][ offset u64 ][ size u64 ][ sha256 32 bytes ]
// ============================================================================

#include <Windows.h>
#include <CommCtrl.h>
#include <ShObjIdl.h>
#include <bcrypt.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Bcrypt.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace {

constexpr uint64_t kPayloadMagic = 0x314D484254ull; // "TBHM1"
const wchar_t* kAppTitle = L"八猴5汉化版安装程序";
constexpr int kInstallButtonId = 1001;
constexpr int kUninstallButtonId = 1002;
constexpr int kDirectoryEditId = 1003;
constexpr int kBrowseButtonId = 1004;

struct InstallerDialogState {
    std::wstring toolbagDirectory;
    int operationId = 0;
    HWND directoryEdit = nullptr;
    UINT dpi = 96;
    HFONT uiFont = nullptr;
};

int ScaleForDpi(int value, UINT dpi) {
    return MulDiv(value, static_cast<int>(dpi), 96);
}

class ComApartment {
public:
    ComApartment() : initialized_(SUCCEEDED(CoInitializeEx(
        nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {}
    ~ComApartment() { if (initialized_) CoUninitialize(); }
    ComApartment(const ComApartment&) = delete;
    ComApartment& operator=(const ComApartment&) = delete;

private:
    bool initialized_;
};

std::wstring GetExecutablePath() {
    std::vector<wchar_t> buffer(1024);
    while (buffer.size() <= 32768) {
        const DWORD length = GetModuleFileNameW(
            nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (!length) return L"";
        if (length < buffer.size() - 1)
            return std::wstring(buffer.data(), length);
        buffer.resize(buffer.size() * 2);
    }
    return L"";
}

std::wstring GetEnvironmentString(const wchar_t* name) {
    const DWORD required = GetEnvironmentVariableW(name, nullptr, 0);
    if (!required) return L"";
    std::vector<wchar_t> buffer(required);
    const DWORD length = GetEnvironmentVariableW(name, buffer.data(), required);
    return length && length < required ? std::wstring(buffer.data(), length) : L"";
}

std::wstring GetControlText(HWND control) {
    const int length = GetWindowTextLengthW(control);
    if (length <= 0) return L"";
    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1);
    const int copied = GetWindowTextW(control, buffer.data(),
                                      static_cast<int>(buffer.size()));
    return copied > 0 ? std::wstring(buffer.data(), copied) : L"";
}

bool IsX64WindowsExecutable(const std::wstring& path) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    IMAGE_DOS_HEADER dos{};
    DWORD read = 0;
    bool valid = ReadFile(file, &dos, sizeof(dos), &read, nullptr) &&
                 read == sizeof(dos) && dos.e_magic == IMAGE_DOS_SIGNATURE &&
                 dos.e_lfanew > 0 &&
                 SetFilePointer(file, dos.e_lfanew, nullptr, FILE_BEGIN) !=
                     INVALID_SET_FILE_POINTER;
    DWORD signature = 0;
    IMAGE_FILE_HEADER header{};
    valid = valid && ReadFile(file, &signature, sizeof(signature), &read, nullptr) &&
            read == sizeof(signature) && signature == IMAGE_NT_SIGNATURE &&
            ReadFile(file, &header, sizeof(header), &read, nullptr) &&
            read == sizeof(header) && header.Machine == IMAGE_FILE_MACHINE_AMD64;
    CloseHandle(file);
    return valid;
}

struct PayloadEntry {
    std::string name;
    uint64_t offset = 0;
    uint64_t size = 0;
    unsigned char sha256[32] = {};
};

// ---------------------------------------------------------------------------
// Embedded-payload reader
// ---------------------------------------------------------------------------
bool ReadPayloadManifest(std::vector<PayloadEntry>& entries) {
    entries.clear();
    const std::wstring executablePath = GetExecutablePath();
    if (executablePath.empty()) return false;

    FILE* file = nullptr;
    if (_wfopen_s(&file, executablePath.c_str(), L"rb") || !file) return false;

    _fseeki64(file, 0, SEEK_END);
    const int64_t fileSize = _ftelli64(file);
    if (fileSize < 16) { fclose(file); return false; }

    uint64_t magic = 0;
    uint32_t count = 0;
    uint32_t manifestSize = 0;
    const bool footerRead =
        _fseeki64(file, fileSize - 8, SEEK_SET) == 0 &&
        fread(&magic, sizeof(magic), 1, file) == 1 &&
        _fseeki64(file, fileSize - 12, SEEK_SET) == 0 &&
        fread(&count, sizeof(count), 1, file) == 1 &&
        _fseeki64(file, fileSize - 16, SEEK_SET) == 0 &&
        fread(&manifestSize, sizeof(manifestSize), 1, file) == 1;
    if (!footerRead || magic != kPayloadMagic || count == 0 || count > 32 ||
        manifestSize > (uint64_t)fileSize - 16) { fclose(file); return false; }

    const int64_t entriesStart = fileSize - 16 - manifestSize;
    _fseeki64(file, entriesStart, SEEK_SET);

    for (uint32_t i = 0; i < count; i++) {
        uint32_t nameLen = 0;
        if (fread(&nameLen, 4, 1, file) != 1 || nameLen == 0 || nameLen > 260) {
            fclose(file); return false;
        }
        std::vector<char> name(nameLen + 1);
        if (fread(name.data(), 1, nameLen, file) != nameLen) {
            fclose(file); return false;
        }
        name[nameLen] = 0;
        PayloadEntry entry;
        entry.name = name.data();
        if (entry.name.find("..") != std::string::npos || entry.name[0] == '/' ||
            entry.name[0] == '\\' || entry.name.find(':') != std::string::npos ||
            fread(&entry.offset, 8, 1, file) != 1 ||
            fread(&entry.size, 8, 1, file) != 1 ||
            fread(entry.sha256, 1, sizeof(entry.sha256), file) != sizeof(entry.sha256) ||
            entry.offset > (uint64_t)entriesStart ||
            entry.size > (uint64_t)entriesStart - entry.offset) {
            fclose(file); return false;
        }
        entries.push_back(std::move(entry));
    }
    fclose(file);
    return entries.size() == count;
}

// ---------------------------------------------------------------------------
// Temp-folder helpers (ASCII paths: temp folder + known file names)
// ---------------------------------------------------------------------------
std::wstring CreateExtractionDirectory() {
    const DWORD required = GetTempPathW(0, nullptr);
    if (!required) return L"";
    std::vector<wchar_t> tempPath(required);
    if (!GetTempPathW(required, tempPath.data())) return L"";
    wchar_t unique[MAX_PATH];
    if (!GetTempFileNameW(tempPath.data(), L"TB5", 0, unique)) return L"";
    DeleteFileW(unique);
    std::wstring folder = std::wstring(unique) + L"\\";
    if (!CreateDirectoryW(folder.c_str(), nullptr)) return L"";
    if (!CreateDirectoryW((folder + L"scripts").c_str(), nullptr) ||
        !CreateDirectoryW((folder + L"dist").c_str(), nullptr)) {
        RemoveDirectoryW((folder + L"scripts").c_str());
        RemoveDirectoryW((folder + L"dist").c_str());
        RemoveDirectoryW(folder.c_str());
        return L"";
    }
    return folder;
}

void DeleteExtractionDirectory(const std::wstring& directory) {
    if (directory.empty()) return;
    for (const auto* f : {L"scripts\\install.ps1", L"dist\\dictionary_zh.json",
                          L"dist\\ToolbagChineseHook.dll",
                          L"dist\\ToolbagChineseLauncher.exe",
                          L"dist\\alibaba_puhuiti.slug", L"dist\\tbscene.ico"}) {
        DeleteFileW((directory + f).c_str());
    }
    RemoveDirectoryW((directory + L"scripts").c_str());
    RemoveDirectoryW((directory + L"dist").c_str());
    RemoveDirectoryW(directory.c_str());
}

bool ExtractPayloadFiles(const std::vector<PayloadEntry>& entries,
                         const std::wstring& directory) {
    const std::wstring executablePath = GetExecutablePath();
    if (executablePath.empty()) return false;
    FILE* in = nullptr;
    if (_wfopen_s(&in, executablePath.c_str(), L"rb") || !in) return false;

    bool ok = true;
    for (const auto& entry : entries) {
        const std::wstring name(entry.name.begin(), entry.name.end());
        const std::wstring destination = directory + name;
        FILE* out = nullptr;
        if (_wfopen_s(&out, destination.c_str(), L"wb") || !out) { ok = false; break; }
        _fseeki64(in, entry.offset, SEEK_SET);
        BCRYPT_ALG_HANDLE algorithm = nullptr;
        BCRYPT_HASH_HANDLE hash = nullptr;
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM,
                                        nullptr, 0) < 0 ||
            BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) < 0) {
            if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
            fclose(out); ok = false; break;
        }
        std::vector<unsigned char> buf(1 << 20);
        uint64_t remaining = entry.size;
        while (remaining > 0) {
            const size_t chunk = remaining > buf.size() ? buf.size() : (size_t)remaining;
            if (fread(buf.data(), 1, chunk, in) != chunk) { ok = false; break; }
            if (fwrite(buf.data(), 1, chunk, out) != chunk) { ok = false; break; }
            if (BCryptHashData(hash, buf.data(), (ULONG)chunk, 0) < 0) { ok = false; break; }
            remaining -= chunk;
        }
        unsigned char digest[32] = {};
        if (ok && (BCryptFinishHash(hash, digest, sizeof(digest), 0) < 0 ||
                   memcmp(digest, entry.sha256, sizeof(digest)) != 0)) ok = false;
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(algorithm, 0);
        fclose(out);
        if (!ok) break;
    }
    fclose(in);
    return ok;
}

bool SelectToolbagDirectory(std::wstring& directory) {
    IFileOpenDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&dialog)))) return false;

    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM |
                       FOS_PATHMUSTEXIST | FOS_DONTADDTORECENT);
    dialog->SetTitle(L"请选择 Toolbag 5 安装目录");

    // Installation always uses the folder picker. Start at Toolbag's standard
    // location when it exists, while still allowing any custom location.
    const std::wstring programFiles = GetEnvironmentString(L"ProgramFiles");
    if (!programFiles.empty()) {
        const std::wstring defaultFolder =
            programFiles + L"\\Marmoset\\Toolbag 5";
        IShellItem* initialItem = nullptr;
        if (SUCCEEDED(SHCreateItemFromParsingName(defaultFolder.c_str(), nullptr,
                                                  IID_PPV_ARGS(&initialItem)))) {
            dialog->SetFolder(initialItem);
            dialog->SetDefaultFolder(initialItem);
            initialItem->Release();
        }
    }

    bool selected = false;
    if (SUCCEEDED(dialog->Show(nullptr))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item))) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                directory = path;
                CoTaskMemFree(path);
                selected = true;
            }
            item->Release();
        }
    }
    dialog->Release();
    return selected;
}

LRESULT CALLBACK InstallerWindowProcedure(HWND window, UINT message,
                                           WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<InstallerDialogState*>(
        GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = reinterpret_cast<InstallerDialogState*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }

    switch (message) {
    case WM_CREATE: {
        state->dpi = GetDpiForWindow(window);
        const auto s = [state](int value) { return ScaleForDpi(value, state->dpi); };
        state->uiFont = CreateFontW(-MulDiv(10, state->dpi, 72), 0, 0, 0,
            FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
            L"Microsoft YaHei UI");
        HWND label = CreateWindowExW(0, L"STATIC", L"Toolbag 目录",
            WS_CHILD | WS_VISIBLE, s(24), s(20), s(120), s(22),
            window, nullptr, nullptr, nullptr);
        state->directoryEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", state->toolbagDirectory.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            s(24), s(45), s(455), s(31), window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDirectoryEditId)),
            nullptr, nullptr);
        HWND browse = CreateWindowExW(0, L"BUTTON", L"浏览…",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            s(489), s(44), s(87), s(33), window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(kBrowseButtonId)),
            nullptr, nullptr);
        HWND install = CreateWindowExW(0, L"BUTTON", L"安装汉化",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            s(324), s(98), s(120), s(36), window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(kInstallButtonId)),
            nullptr, nullptr);
        HWND uninstall = CreateWindowExW(0, L"BUTTON", L"拆卸汉化",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            s(456), s(98), s(120), s(36), window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(kUninstallButtonId)),
            nullptr, nullptr);
        for (HWND control : {label, state->directoryEdit, browse, install, uninstall})
            SendMessageW(control, WM_SETFONT,
                         reinterpret_cast<WPARAM>(state->uiFont), TRUE);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
        return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
    }
    case WM_COMMAND: {
        if (!state) return 0;
        const int id = LOWORD(wParam);
        if (id == kBrowseButtonId) {
            std::wstring selected = GetControlText(state->directoryEdit);
            if (SelectToolbagDirectory(selected))
                SetWindowTextW(state->directoryEdit, selected.c_str());
            return 0;
        }
        if (id == kInstallButtonId || id == kUninstallButtonId) {
            state->toolbagDirectory = GetControlText(state->directoryEdit);
            while (!state->toolbagDirectory.empty() &&
                   (state->toolbagDirectory.back() == L' ' ||
                    state->toolbagDirectory.back() == L'\\'))
                state->toolbagDirectory.pop_back();
            const std::wstring exe = state->toolbagDirectory + L"\\toolbag.exe";
            const DWORD attributes = GetFileAttributesW(exe.c_str());
            if (state->toolbagDirectory.empty() ||
                attributes == INVALID_FILE_ATTRIBUTES ||
                (attributes & FILE_ATTRIBUTE_DIRECTORY) ||
                !IsX64WindowsExecutable(exe)) {
                MessageBoxW(window,
                    L"所选目录不是有效的 Toolbag 5 安装目录。\n\n"
                    L"请选择包含 toolbag.exe 的目录。",
                    kAppTitle, MB_OK | MB_ICONERROR);
                SetFocus(state->directoryEdit);
                return 0;
            }
            state->operationId = id;
            DestroyWindow(window);
            return 0;
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        if (state) {
            if (state->uiFont) DeleteObject(state->uiFont);
            state->uiFont = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

int ShowInstallerWindow(HINSTANCE instance, InstallerDialogState& state) {
    const wchar_t* className = L"ToolbagChineseInstallerWindow";
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = InstallerWindowProcedure;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(101));
    wc.hIconSm = wc.hIcon;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = className;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return 0;

    const UINT dpi = GetDpiForSystem();
    RECT frame = {0, 0, ScaleForDpi(600, dpi), ScaleForDpi(155, dpi)};
    AdjustWindowRectExForDpi(&frame, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                             FALSE, WS_EX_DLGMODALFRAME, dpi);
    const int width = frame.right - frame.left;
    const int height = frame.bottom - frame.top;
    const int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME, className, kAppTitle,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, y, width, height, nullptr, nullptr, instance, &state);
    if (!window) return 0;
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    MSG message = {};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    return state.operationId;
}

} // namespace

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE,
                      _In_ PWSTR, _In_ int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    ComApartment comApartment;
    std::vector<PayloadEntry> payloadEntries;
    if (!ReadPayloadManifest(payloadEntries)) {
        MessageBoxW(nullptr, L"安装程序缺少必要数据，请重新下载完整版本。",
                    kAppTitle, MB_OK | MB_ICONERROR);
        return 1;
    }

    InstallerDialogState dialogState;
    dialogState.toolbagDirectory =
        GetEnvironmentString(L"ProgramFiles") + L"\\Marmoset\\Toolbag 5";
    const int operationId = ShowInstallerWindow(instance, dialogState);
    if (operationId == 0) return 0;
    const bool uninstall = operationId == kUninstallButtonId;

    const std::wstring extractionDirectory = CreateExtractionDirectory();
    if (extractionDirectory.empty() ||
        !ExtractPayloadFiles(payloadEntries, extractionDirectory)) {
        MessageBoxW(nullptr, L"解压安装数据失败，请重试。", kAppTitle, MB_OK | MB_ICONERROR);
        DeleteExtractionDirectory(extractionDirectory);
        return 2;
    }

    const std::wstring installScript = extractionDirectory + L"scripts\\install.ps1";
    std::wstring commandLine =
        L"powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" +
        installScript + L"\" -Quiet";
    if (uninstall) commandLine += L" -Uninstall";
    commandLine += L" -ToolbagDir \"" + dialogState.toolbagDirectory + L"\"";
    std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back(L'\0');

    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};

    int exitCode = 3;
    if (CreateProcessW(nullptr, mutableCommandLine.data(), nullptr, nullptr,
                       FALSE, CREATE_NO_WINDOW,
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
        DeleteExtractionDirectory(extractionDirectory);
        return 3;
    }

    DeleteExtractionDirectory(extractionDirectory);

    // Result
    if (exitCode == 0) {
        MessageBoxW(nullptr,
                    uninstall
                        ? L"拆卸完成。\n\n插件文件和快捷方式已删除，字体及文件关联已恢复。"
                        : L"安装完成。\n\n请通过“八猴5汉化版”快捷方式启动。\n\n"
                          L"首次双击 .tbscene 时，请选择“八猴5汉化版”并点击“始终”。",
                    kAppTitle, MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    // install.ps1 displays one detailed error dialog (including its log path).
    // Avoid following it with a second generic and unhelpful popup.
    return exitCode;
}
