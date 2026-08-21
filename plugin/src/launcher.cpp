// ============================================================================
//  Toolbag 5 Chinese Localizer - Launcher
// ============================================================================
//  Toolbag itself does not support runtime injection, so this launcher:
//    1. starts toolbag.exe in a suspended state
//    2. injects ToolbagChineseHook.dll via a remote LoadLibraryW thread
//    3. waits for the DLL to signal that hooks were installed
//    4. resumes the main thread (or aborts cleanly if installation failed)
//  The original program files are never modified.
// ============================================================================

#include <Windows.h>
#include <string>

namespace {

// Returns the directory component of a path (everything before the last \ or /).
std::wstring GetDirectory(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"" : path.substr(0, slash);
}

void ShowError(const wchar_t* message) {
    MessageBoxW(nullptr, message, L"八猴 Toolbag 5 汉化启动器", MB_OK | MB_ICONERROR);
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    // --- Locate files -------------------------------------------------------
    wchar_t ownPath[MAX_PATH];
    GetModuleFileNameW(nullptr, ownPath, MAX_PATH);

    const std::wstring pluginDir = GetDirectory(ownPath);
    const std::wstring dllPath   = pluginDir + L"\\ToolbagChineseHook.dll";
    const std::wstring toolbagRoot = GetDirectory(GetDirectory(GetDirectory(pluginDir)));
    const std::wstring exePath   = toolbagRoot + L"\\toolbag.exe";

    if (GetFileAttributesW(exePath.c_str()) == INVALID_FILE_ATTRIBUTES ||
        GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        ShowError(L"找不到 Toolbag 或汉化 DLL，请重新安装汉化插件。");
        return 1;
    }

    // --- Start Toolbag suspended -------------------------------------------
    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};

    if (!CreateProcessW(exePath.c_str(), nullptr, nullptr, nullptr, FALSE,
                        CREATE_SUSPENDED, nullptr, toolbagRoot.c_str(),
                        &startupInfo, &processInfo)) {
        ShowError(L"无法启动 Toolbag。");
        return 2;
    }

    // --- Inject the hook DLL ------------------------------------------------
    wchar_t readyEventName[96];
    swprintf_s(readyEventName, L"Local\\ToolbagChineseHookReady_%lu",
               processInfo.dwProcessId);
    HANDLE readyEvent = CreateEventW(nullptr, TRUE, FALSE, readyEventName);

    const SIZE_T dllPathBytes = (dllPath.size() + 1) * sizeof(wchar_t);
    void* remoteDllPath = VirtualAllocEx(processInfo.hProcess, nullptr, dllPathBytes,
                                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    SIZE_T written = 0;
    const bool injected = readyEvent && remoteDllPath &&
        WriteProcessMemory(processInfo.hProcess, remoteDllPath, dllPath.c_str(),
                           dllPathBytes, &written);

    const auto loadLibrary =
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),
                                               "LoadLibraryW");
    HANDLE remoteThread = injected
        ? CreateRemoteThread(processInfo.hProcess, nullptr, 0, loadLibrary,
                             remoteDllPath, 0, nullptr)
        : nullptr;

    bool hookReady = false;
    if (remoteThread) {
        WaitForSingleObject(remoteThread, 10000);
        CloseHandle(remoteThread);
        hookReady = WaitForSingleObject(readyEvent, 10000) == WAIT_OBJECT_0;
    }

    // --- Finish: resume or abort --------------------------------------------
    if (hookReady) {
        ResumeThread(processInfo.hThread);
    } else {
        TerminateProcess(processInfo.hProcess, 3);
        ShowError(L"汉化 Hook 安装失败。Toolbag 未启动，原程序文件未受影响。");
    }

    if (remoteDllPath) VirtualFreeEx(processInfo.hProcess, remoteDllPath, 0, MEM_RELEASE);
    if (readyEvent) CloseHandle(readyEvent);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);

    return hookReady ? 0 : 3;
}
