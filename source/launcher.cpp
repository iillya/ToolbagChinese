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
#include <TlHelp32.h>
#include <string>
#include <vector>

namespace {

// Returns the directory component of a path (everything before the last \ or /).
std::wstring GetParentDirectory(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"" : path.substr(0, slash);
}

void ShowLauncherError(const wchar_t* message) {
    MessageBoxW(nullptr, message, L"八猴5汉化版", MB_OK | MB_ICONERROR);
}

LPTHREAD_START_ROUTINE ResolveLoadLibraryEntryPoint(DWORD processId) {
    HMODULE localKernel = GetModuleHandleW(L"kernel32.dll");
    if (!localKernel) return nullptr;
    FARPROC localFunction = GetProcAddress(localKernel, "LoadLibraryW");
    if (!localFunction) return nullptr;

    const uintptr_t functionRva = reinterpret_cast<uintptr_t>(localFunction) -
                                  reinterpret_cast<uintptr_t>(localKernel);
    HANDLE snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W module{};
        module.dwSize = sizeof(module);
        if (Module32FirstW(snapshot, &module)) {
            do {
                if (_wcsicmp(module.szModule, L"kernel32.dll") == 0) {
                    CloseHandle(snapshot);
                    return reinterpret_cast<LPTHREAD_START_ROUTINE>(
                        reinterpret_cast<uintptr_t>(module.modBaseAddr) +
                        functionRva);
                }
            } while (Module32NextW(snapshot, &module));
        }
        CloseHandle(snapshot);
    }
    // Never pass a function address from this process to another process.
    // ASLR normally keeps system DLL bases aligned, but that is not an API
    // guarantee; failing cleanly is safer than starting a thread at an
    // unverified address.
    return nullptr;
}

std::wstring GetExecutablePath() {
    std::vector<wchar_t> buffer(1024);
    while (buffer.size() <= 32768) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                                (DWORD)buffer.size());
        if (!length) return L"";
        if (length < buffer.size() - 1) return std::wstring(buffer.data(), length);
        buffer.resize(buffer.size() * 2);
    }
    return L"";
}

bool IsRegularFile(const std::wstring& path) {
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring FindToolbagRoot(const std::wstring& startDirectory) {
    std::wstring directory = startDirectory;
    for (int depth = 0; depth < 10; ++depth) {
        if (IsRegularFile(directory + L"\\toolbag.exe")) return directory;
        const std::wstring parent = GetParentDirectory(directory);
        if (parent.empty() || parent == directory) break;
        directory = parent;
    }
    return L"";
}

} // namespace

int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE,
                      _In_ PWSTR commandLine, _In_ int) {
    // --- Locate files -------------------------------------------------------
    const std::wstring ownPath = GetExecutablePath();
    const std::wstring pluginDir = GetParentDirectory(ownPath);
    const std::wstring dllPath   = pluginDir + L"\\ToolbagChineseHook.dll";

    // Find Toolbag's root by walking up from the launcher's own folder until
    // toolbag.exe is found.  The launcher is no longer tied to data\plugin\,
    // so it can live anywhere (and no longer creates a plugin-menu entry).
    const std::wstring toolbagRoot = FindToolbagRoot(pluginDir);
    const std::wstring exePath = toolbagRoot + L"\\toolbag.exe";

    if (toolbagRoot.empty() || !IsRegularFile(dllPath)) {
        ShowLauncherError(L"找不到 Toolbag 或汉化 DLL，请重新安装汉化插件。");
        return 1;
    }

    // --- Start Toolbag suspended -------------------------------------------
    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};

    // Preserve file paths and any other arguments handed to this launcher.
    // This is required when Windows opens a .tbscene through the localized
    // launcher: Toolbag must receive the original scene path unchanged.
    std::wstring childCommandLine = L"\"" + exePath + L"\"";
    if (commandLine && *commandLine) {
        childCommandLine += L" ";
        childCommandLine += commandLine;
    }
    std::vector<wchar_t> mutableCommandLine(childCommandLine.begin(),
                                             childCommandLine.end());
    mutableCommandLine.push_back(L'\0');

    if (!CreateProcessW(exePath.c_str(), mutableCommandLine.data(),
                        nullptr, nullptr, FALSE,
                        CREATE_SUSPENDED, nullptr, toolbagRoot.c_str(),
                        &startupInfo, &processInfo)) {
        ShowLauncherError(L"无法启动 Toolbag。");
        return 2;
    }

    // --- Inject the hook DLL ------------------------------------------------
    wchar_t readyEventName[96];
    swprintf_s(readyEventName, L"Local\\ToolbagChineseHookReady_%lu",
               processInfo.dwProcessId);
    HANDLE readyEvent = CreateEventW(nullptr, TRUE, FALSE, readyEventName);

    wchar_t startupInfoName[96];
    swprintf_s(startupInfoName, L"Local\\ToolbagChineseStartup_%lu",
               processInfo.dwProcessId);
    HANDLE startupMapping = CreateFileMappingW(
        INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(DWORD),
        startupInfoName);
    DWORD* sharedMainThreadId = startupMapping
        ? static_cast<DWORD*>(MapViewOfFile(startupMapping, FILE_MAP_WRITE,
                                            0, 0, sizeof(DWORD)))
        : nullptr;
    if (sharedMainThreadId) *sharedMainThreadId = processInfo.dwThreadId;

    const SIZE_T dllPathBytes = (dllPath.size() + 1) * sizeof(wchar_t);
    void* remoteDllPath = VirtualAllocEx(processInfo.hProcess, nullptr, dllPathBytes,
                                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    SIZE_T written = 0;
    const bool startupInfoReady = sharedMainThreadId != nullptr;
    const bool dllPathWritten = readyEvent && startupInfoReady && remoteDllPath &&
        WriteProcessMemory(processInfo.hProcess, remoteDllPath, dllPath.c_str(),
                           dllPathBytes, &written) && written == dllPathBytes;

    const auto loadLibrary = ResolveLoadLibraryEntryPoint(processInfo.dwProcessId);
    HANDLE remoteThread = nullptr;
    if (dllPathWritten && loadLibrary) {
        remoteThread = CreateRemoteThread(processInfo.hProcess, nullptr, 0,
                                          loadLibrary, remoteDllPath, 0, nullptr);
    }
    const bool remoteThreadCreated = remoteThread != nullptr;

    bool hookReady = false;
    DWORD remoteLoadResult = 0;
    DWORD remoteWaitResult = WAIT_FAILED;
    DWORD readyWaitResult = WAIT_FAILED;
    if (remoteThread) {
        constexpr DWORD kStartupTimeoutMs = 30000;
        remoteWaitResult = WaitForSingleObject(remoteThread, kStartupTimeoutMs);
        if (remoteWaitResult == WAIT_OBJECT_0)
            GetExitCodeThread(remoteThread, &remoteLoadResult);
        CloseHandle(remoteThread);
        if (remoteWaitResult == WAIT_OBJECT_0 && remoteLoadResult != 0) {
            readyWaitResult = WaitForSingleObject(readyEvent, kStartupTimeoutMs);
            hookReady = readyWaitResult == WAIT_OBJECT_0;
        }
    }

    // --- Finish: resume or abort --------------------------------------------
    if (hookReady && ResumeThread(processInfo.hThread) != static_cast<DWORD>(-1)) {
        hookReady = true;
    } else {
        hookReady = false;
        TerminateProcess(processInfo.hProcess, 3);
        WaitForSingleObject(processInfo.hProcess, 5000);
        wchar_t detail[640] = {};
        swprintf_s(detail,
            L"汉化 Hook 安装失败。Toolbag 未启动，原程序文件未受影响。\n\n"
            L"诊断：事件=%s，主线程信息=%s，内存=%s，写入=%s，"
            L"远程地址=%s，远程线程=%s，"
            L"远程等待=0x%08lX，加载结果=0x%08lX，Hook等待=0x%08lX",
            readyEvent ? L"成功" : L"失败",
            startupInfoReady ? L"成功" : L"失败",
            remoteDllPath ? L"成功" : L"失败",
            dllPathWritten ? L"成功" : L"失败",
            loadLibrary ? L"成功" : L"失败",
            remoteThreadCreated ? L"成功" : L"失败",
            remoteWaitResult, remoteLoadResult, readyWaitResult);
        ShowLauncherError(detail);
    }

    // The remote thread can still be reading this path after a timeout.  The
    // failure path terminates the child, so let process teardown reclaim it.
    if (remoteDllPath && remoteWaitResult == WAIT_OBJECT_0)
        VirtualFreeEx(processInfo.hProcess, remoteDllPath, 0, MEM_RELEASE);
    if (sharedMainThreadId) UnmapViewOfFile(sharedMainThreadId);
    if (startupMapping) CloseHandle(startupMapping);
    if (readyEvent) CloseHandle(readyEvent);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);

    return hookReady ? 0 : 3;
}
