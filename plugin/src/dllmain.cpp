// ============================================================================
//  Toolbag 5 Chinese Localizer - Hook DLL
// ============================================================================
//  Runtime UI translation for Marmoset Toolbag 5.
//
//  It hooks three engine entry points so that text is translated *in memory*
//  without touching Toolbag's assets/paths/index/configuration:
//    * Text::setText            -> ordinary control text
//    * Menu::MenuItem label     -> menu titles only (commands/callbacks kept)
//    * Font::CompiledString     -> self-drawn text (Slug) incl. glyph metrics
//
//  A GDI/USER32 IAT capture safety-net records any string that is drawn via
//  standard GDI APIs but is missing from the dictionary (so nothing is missed).
//
//  Dictionary loaded next to this DLL:
//    * dictionary_zh.json  -> merged UI + asset translations (sp-translation-v1)
// ============================================================================

#include <Windows.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <wchar.h>
#include <intrin.h>

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------
using FontCompileFn   = void(__fastcall*)(void*, const char*);
using StringAssignFn  = void*(__fastcall*)(void*, const char*, size_t);

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
static HMODULE          g_hookModule = nullptr;
static FontCompileFn    g_originalFontCompile = nullptr;
static StringAssignFn   g_originalStringAssign = nullptr;
static size_t           g_textStringOffset1 = 0;
static size_t           g_textStringOffset2 = 0;


static BYTE*            g_imageBegin = nullptr;
static BYTE*            g_imageEnd = nullptr;
static thread_local uintptr_t g_callsite = 0;

static std::unordered_map<std::string,std::string> g_translationDict;
static std::unordered_map<std::string,std::string> g_displayCache;
static std::mutex       g_cacheLock;

static std::unordered_set<std::string> g_loggedMissing;
static std::mutex       g_missingLock;

static std::string      g_missingLogPath;
static std::string      g_traceLogPath;
static bool             g_traceEnabled = false;

static INIT_ONCE        g_initOnce = INIT_ONCE_STATIC_INIT;
static BOOL             g_hooksInstalled = FALSE;

// ---------------------------------------------------------------------------
// String helpers
// ---------------------------------------------------------------------------
static std::string Trim(const std::string& s) {
    const size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

// ---------------------------------------------------------------------------
// Dictionary loading
// ---------------------------------------------------------------------------
// Minimal JSON string unescaper ("... \n \t \\ \" \uXXXX ...").
static std::string JsonUnescape(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); i++) {
        char c = in[i];
        if (c != '\\' || i + 1 >= in.size()) { out += c; continue; }
        char e = in[++i];
        switch (e) {
            case 'n': out += '\n'; break;
            case 't': out += '\t'; break;
            case 'r': out += '\r'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'u': {
                if (i + 4 < in.size()) {
                    unsigned cp = 0;
                    for (int j = 1; j <= 4; j++) {
                        char h = in[i + j]; cp <<= 4;
                        if (h >= '0' && h <= '9') cp |= (unsigned)(h - '0');
                        else if (h >= 'a' && h <= 'f') cp |= (unsigned)(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') cp |= (unsigned)(h - 'A' + 10);
                    }
                    i += 4;
                    if (cp < 0x80) out += (char)cp;
                    else if (cp < 0x800) {
                        out += (char)(0xC0 | (cp >> 6));
                        out += (char)(0x80 | (cp & 0x3F));
                    } else {
                        out += (char)(0xE0 | (cp >> 12));
                        out += (char)(0x80 | ((cp >> 6) & 0x3F));
                        out += (char)(0x80 | (cp & 0x3F));
                    }
                }
                break;
            }
            default: out += e; break;
        }
    }
    return out;
}

// Read a quoted JSON string starting after the opening quote. Stops before the
// closing quote; handles \" escapes so they don't terminate early.
static std::string ReadJsonString(const std::string& text, size_t& pos) {
    std::string out;
    while (pos < text.size()) {
        char c = text[pos];
        if (c == '\\' && pos + 1 < text.size()) {
            out += c; out += text[pos + 1]; pos += 2; continue;
        }
        if (c == '"') { pos++; break; }
        out += c; pos++;
    }
    return JsonUnescape(out);
}

// Load a sp-translation-v1 JSON dictionary (format like my_assets_zh.json):
// { ..., "translations": { "English": "中文", ... } }.
static bool LoadDictionaryJson(const char* fileName) {
    char path[MAX_PATH];
    const DWORD len = GetModuleFileNameA(g_hookModule, path, MAX_PATH);
    if (!len || len >= MAX_PATH) return false;
    char* slash = strrchr(path, '\\');
    if (!slash) return false;
    strcpy_s(slash + 1, MAX_PATH - (slash - path + 1), fileName);

    FILE* file = nullptr;
    if (fopen_s(&file, path, "rb") || !file) return false;
    fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size <= 0 || size > 64 * 1024 * 1024) { fclose(file); return false; }
    std::string text((size_t)size, '\0');
    if (fread(&text[0], 1, (size_t)size, file) != (size_t)size) { fclose(file); return false; }
    fclose(file);

    bool any = false;
    size_t pos = text.find("\"translations\"");
    if (pos == std::string::npos) return false;
    pos = text.find('{', pos);
    if (pos == std::string::npos) return false;
    pos++;

    while (pos < text.size()) {
        while (pos < text.size() &&
               (text[pos]==' '||text[pos]=='\t'||text[pos]=='\r'||text[pos]=='\n'||text[pos]==','))
            pos++;
        if (pos >= text.size()) break;
        if (text[pos] == '}') break;                 // end of translations object
        if (text[pos] != '"') { pos++; continue; }
        pos++;                                        // opening quote of key
        std::string key = ReadJsonString(text, pos);
        while (pos < text.size() && (text[pos]==' '||text[pos]=='\t'||text[pos]=='\r'||text[pos]=='\n')) pos++;
        if (pos < text.size() && text[pos] == ':') pos++;
        while (pos < text.size() && (text[pos]==' '||text[pos]=='\t'||text[pos]=='\r'||text[pos]=='\n')) pos++;
        if (pos >= text.size() || text[pos] != '"') break;
        pos++;                                        // opening quote of value
        std::string val = ReadJsonString(text, pos);
        if (!key.empty() && !val.empty()) {
            g_translationDict[key] = val;
            any = true;
        }
    }
    return any;
}

static bool LoadDictionaries() {
    return LoadDictionaryJson("dictionary_zh.json");
}

// ---------------------------------------------------------------------------
// Missing / trace logging
// ---------------------------------------------------------------------------
// Records an English string that was shown in the UI but is not in the
// dictionary, so it can be added later. 'kind' marks the source (CTOR/GDI...).
static void LogMissingString(const char* kind, const char* text, size_t length) {
    if (!text || length < 2 || length > 256) return;

    // Only printable ASCII containing at least one letter; skip paths/URLs/etc.
    bool hasLetter = false;
    for (size_t i = 0; i < length; i++) {
        const unsigned char c = text[i];
        if (c == '/' || c == '\\' || c < 0x20 || c > 0x7e) return;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) hasLetter = true;
    }
    if (!hasLetter) return;

    const std::string str(text, length);
    const std::string dedupKey = std::string(kind) + "\t" + str;

    {
        std::lock_guard<std::mutex> lock(g_missingLock);
        if (!g_loggedMissing.insert(dedupKey).second) return;
    }

    const uintptr_t imageBase = (uintptr_t)GetModuleHandleW(nullptr);
    FILE* file = nullptr;
    if (!g_missingLogPath.empty() && !fopen_s(&file, g_missingLogPath.c_str(), "ab") && file) {
        fprintf(file, "%s\t0x%llX\t%s\r\n",
                kind, (unsigned long long)(g_callsite - imageBase), str.c_str());
        fclose(file);
    }
}

// ---------------------------------------------------------------------------
// Translation core
// ---------------------------------------------------------------------------
// Some strings are sentinel values, not user-facing text to localize.
// Translating them (e.g. "None" -> "无") breaks controls that use them as a
// "no selection / NULL" marker (combo boxes drop the text / show blank).
static bool IsSentinelText(const char* s) {
    if (!s || !*s) return true;              // empty / null
    if (_stricmp(s, "None") == 0) return true;
    if (_stricmp(s, "NULL") == 0) return true;
    return false;
}

// Translate 's' using the dictionary. Preserves surrounding whitespace and
// caches the result so the same string is not rebuilt repeatedly.
// SAFETY: returns a pointer into g_displayCache (std::unordered_map). The map is
// node-based and entries are never erased/replaced, so returned pointers stay
// valid for the lifetime of the process (engine may hold them).
static const char* TranslateImpl(const char* s) {
    if (!s || !*s) return s;

    const std::string whole(s);
    const size_t first = whole.find_first_not_of(" \t");
    const size_t last  = whole.find_last_not_of(" \t");
    if (first == std::string::npos) return s;

    const std::string key = whole.substr(first, last - first + 1);
    if (IsSentinelText(key.c_str())) return s;   // keep sentinels as-is
    const auto it = g_translationDict.find(key);
    if (it == g_translationDict.end()) {
        LogMissingString("CTOR", key.c_str(), key.size());
        return s;
    }

    {
        std::lock_guard<std::mutex> lock(g_cacheLock);
        const auto cached = g_displayCache.find(whole);
        if (cached != g_displayCache.end()) return cached->second.c_str();

        std::string out = whole.substr(0, first) + it->second + whole.substr(last + 1);
        const auto inserted = g_displayCache.emplace(std::move(whole), std::move(out));
        return inserted.first->second.c_str();
    }
}

static const char* Translate(const char* s) {
    __try {
        return TranslateImpl(s);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return s;
    }
}

// ---------------------------------------------------------------------------
// Engine hooks (replacement functions)
// ---------------------------------------------------------------------------
// Lightweight per-hook trace (enabled by trace.enabled next to the DLL).
static void TraceHook(const char* kind, const char* text) {
    if (!g_traceEnabled || !text) return;
    void* frames[10] = {};
    const USHORT count = CaptureStackBackTrace(0, _countof(frames), frames, nullptr);
    const uintptr_t imageBase = (uintptr_t)GetModuleHandleW(nullptr);
    FILE* f = nullptr;
    if (fopen_s(&f, g_traceLogPath.c_str(), "ab") || !f) return;
    fprintf(f, "HOOK\t%s\t%s\tlen=%zu\t", kind, text, strlen(text));
    for (USHORT i = 0; i < count; i++) {
        const uintptr_t p = (uintptr_t)frames[i];
        if (p >= imageBase && p < (uintptr_t)g_imageEnd)
            fprintf(f, "+0x%llX,", (unsigned long long)(p - imageBase));
        else
            fprintf(f, "@0x%llX,", (unsigned long long)p);
    }
    fprintf(f, "\n");
    fclose(f);
}

// Font::CompiledString - translate before the compiled result is built/measured.
static void HandleFontCompile(void* self, const char* text) {
    g_callsite = (uintptr_t)_ReturnAddress();
    TraceHook("FONT", text);
    const char* out = Translate(text);
    TraceHook("FONT->", out);
    g_originalFontCompile(self, out);
}

// ---------------------------------------------------------------------------
// GDI / USER32 capture safety-net (IAT hooks)
// ---------------------------------------------------------------------------
static void LogGdiString(const wchar_t* text, size_t length) {
    if (!text || length == 0 || length > 512) return;
    char buf[2048];
    const int len = WideCharToMultiByte(CP_UTF8, 0, text, (int)length,
                                        buf, (int)sizeof(buf) - 1, nullptr, nullptr);
    if (len <= 0 || len > 512) return;
    buf[len] = 0;
    LogMissingString("GDI", buf, len);
}

static BOOL (WINAPI* g_origExtTextOutW)(HDC,int,int,UINT,const RECT*,LPCWSTR,UINT,const INT*);
static BOOL WINAPI HookExtTextOutW(HDC hdc,int x,int y,UINT options,const RECT* rect,
                                   LPCWSTR text,UINT count,const INT* dx) {
    g_callsite = (uintptr_t)_ReturnAddress();
    LogGdiString(text, count);
    return g_origExtTextOutW(hdc, x, y, options, rect, text, count, dx);
}

static int (WINAPI* g_origDrawTextW)(HDC,LPCWSTR,int,LPRECT,UINT);
static int WINAPI HookDrawTextW(HDC hdc,LPCWSTR text,int count,LPRECT rect,UINT format) {
    g_callsite = (uintptr_t)_ReturnAddress();
    LogGdiString(text, count < 0 ? wcslen(text) : (size_t)count);
    return g_origDrawTextW(hdc, text, count, rect, format);
}

static BOOL (WINAPI* g_origSetWindowTextW)(HWND,LPCWSTR);
static const wchar_t kAuthorSuffix[] = L" - Bilibili神说要凑数汉化";
static bool g_titlePatched = false;
static BOOL WINAPI HookSetWindowTextW(HWND hwnd,LPCWSTR text) {
    g_callsite = (uintptr_t)_ReturnAddress();
    LogGdiString(text, wcslen(text));
    // Append author signature to the main Toolbag window title (once).
    // Main window is identified by its title (avoids extra user32 imports).
    if (text && text[0] && !g_titlePatched &&
        wcsstr(text, L"Marmoset Toolbag") != nullptr &&
        wcsstr(text, L"Bilibili") == nullptr) {
        const size_t len = wcslen(text);
        const size_t suf = wcslen(kAuthorSuffix);
        wchar_t* buf = (wchar_t*)_alloca((len + suf + 1) * sizeof(wchar_t));
        wcscpy_s(buf, len + suf + 1, text);
        wcscat_s(buf, len + suf + 1, kAuthorSuffix);
        g_titlePatched = true;
        return g_origSetWindowTextW(hwnd, buf);
    }
    return g_origSetWindowTextW(hwnd, text);
}

static BOOL (WINAPI* g_origSetWindowTextA)(HWND,LPCSTR);
static BOOL WINAPI HookSetWindowTextA(HWND hwnd,LPCSTR text) {
    g_callsite = (uintptr_t)_ReturnAddress();
    if (text) {
        const int len = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0) - 1;
        if (len > 0) {
            wchar_t buf[256];
            if (MultiByteToWideChar(CP_UTF8, 0, text, len, buf, 256) > 0)
                LogGdiString(buf, len);
        }
    }
    return g_origSetWindowTextA(hwnd, text);
}

// Patch the Import Address Table of the target module so only Toolbag's direct
// calls go through our hooks (avoids affecting other processes).
static bool PatchImport(HMODULE mod, const char* dllName, const char* funcName,
                        void* hook, FARPROC& original) {
    BYTE* base = (BYTE*)mod;
    if (!base) return false;

    auto* dos = (IMAGE_DOS_HEADER*)base;
    auto* nt  = (IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
    const auto dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dir.VirtualAddress) return false;

    auto* desc = (IMAGE_IMPORT_DESCRIPTOR*)(base + dir.VirtualAddress);
    for (; desc->Name; desc++) {
        const char* importDll = (const char*)(base + desc->Name);
        if (_stricmp(importDll, dllName) != 0) continue;

        const IMAGE_THUNK_DATA64* names =
            (const IMAGE_THUNK_DATA64*)(base + (desc->OriginalFirstThunk ? desc->OriginalFirstThunk : desc->FirstThunk));
        IMAGE_THUNK_DATA64* iat = (IMAGE_THUNK_DATA64*)(base + desc->FirstThunk);

        for (; names->u1.AddressOfData; names++, iat++) {
            if (names->u1.AddressOfData & 0x8000000000000000ull) continue; // ordinal import
            auto* imp = (IMAGE_IMPORT_BY_NAME*)(base + names->u1.AddressOfData);
            if (_stricmp((const char*)imp->Name, funcName) != 0) continue;

            original = (FARPROC)iat->u1.Function;
            DWORD oldProtect;
            VirtualProtect(&iat->u1.Function, sizeof(void*), PAGE_READWRITE, &oldProtect);
            iat->u1.Function = (ULONGLONG)hook;
            VirtualProtect(&iat->u1.Function, sizeof(void*), oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), &iat->u1.Function, sizeof(void*));
            return true;
        }
    }
    return false;
}

static void InstallGdiCaptureHooks(HMODULE mod) {
    FARPROC original = nullptr;
    if (PatchImport(mod, "GDI32.dll", "ExtTextOutW",   (void*)HookExtTextOutW,    original))
        g_origExtTextOutW = (decltype(g_origExtTextOutW))original;
    if (PatchImport(mod, "USER32.dll", "DrawTextW",    (void*)HookDrawTextW,      original))
        g_origDrawTextW = (decltype(g_origDrawTextW))original;
    if (PatchImport(mod, "USER32.dll", "SetWindowTextW",(void*)HookSetWindowTextW,original))
        g_origSetWindowTextW = (decltype(g_origSetWindowTextW))original;
    if (PatchImport(mod, "USER32.dll", "SetWindowTextA",(void*)HookSetWindowTextA,original))
        g_origSetWindowTextA = (decltype(g_origSetWindowTextA))original;
}

// ---------------------------------------------------------------------------
// RTTI / vtable based locating (version-stable primary method).
// Locates functions from the RTTI type-name + vtable instead of byte patterns,
// so a Toolbag update that reorders/relocates code does not break the hooks.
// ---------------------------------------------------------------------------
static DWORD FindBytesInImage(BYTE* base, const void* pat, size_t len, DWORD from = 0) {
    auto* dos = (IMAGE_DOS_HEADER*)base;
    auto* nt  = (IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
    const size_t size = nt->OptionalHeader.SizeOfImage;
    auto* p = (const BYTE*)pat;
    for (size_t i = from; i + len <= size; i++)
        if (!memcmp(base + i, p, len)) return (DWORD)i;
    return 0;
}
static DWORD FindU32InImage(BYTE* base, DWORD val, DWORD from = 0) {
    return FindBytesInImage(base, &val, 4, from);
}
static DWORD FindU64InImage(BYTE* base, uintptr_t val, DWORD from = 0) {
    return FindBytesInImage(base, &val, 8, from);
}

// Resolve a class' primary vftable from its RTTI mangled type name.
static BYTE* ResolveVtableByName(BYTE* base, const char* rttiName) {
    const DWORD nameRva = FindBytesInImage(base, rttiName, (DWORD)strlen(rttiName) + 1);
    if (!nameRva) return nullptr;
    const DWORD td = nameRva - 0x10;                 // TypeDescriptor
    DWORD from = 0;
    while (true) {
        const DWORD ref = FindU32InImage(base, td, from);
        if (!ref) break;
        from = ref + 1;
        const DWORD col = ref - 12;                  // CompleteObjectLocator
        BYTE* cp = base + col;
        const DWORD sig = *(DWORD*)(cp + 0x00);
        const DWORD ptd = *(DWORD*)(cp + 0x0C);
        if (sig > 1 || ptd != td) continue;
        const uintptr_t colPtr = (uintptr_t)base + col;
        const DWORD hit = FindU64InImage(base, colPtr);   // vftable[-1]
        if (hit) return base + hit + 8;              // vftable
    }
    return nullptr;
}

static void EmitJump(BYTE* p, const void* target);

static bool InstallFontHook(BYTE* target);
// Scan a function body for calls to "compile-string" shaped functions
// (test rdx,rdx ; je rel32 ...) and install the Font hook on the first that
// accepts our trampoline. Returns true on success.
static bool InstallFontHookViaVtable(BYTE* slot16, BYTE* imageBegin, BYTE* imageEnd) {
    bool any = false;
    BYTE* end = slot16 + 0x400; if (end > imageEnd) end = imageEnd;
    for (BYTE* p = slot16; p + 5 <= end; p++) {
        if (p[0] != 0xE8) continue;
        const int32_t rel = *(int32_t*)(p + 1);
        BYTE* target = p + 5 + rel;
        if (target >= imageBegin && target < imageEnd &&
            target[0] == 0x48 && target[1] == 0x85 && target[2] == 0xd2 &&
            target[3] == 0x0f && target[4] == 0x84) {
            if (InstallFontHook(target)) any = true;
        }
    }
    return any;
}

// Emit an absolute jump (mov rax, addr; jmp rax) at 'p' targeting 'target'.
static void EmitJump(BYTE* p, const void* target) {
    p[0] = 0x48;                 // REX.W
    p[1] = 0xb8;                 // mov rax, imm64
    *(const void**)(p + 2) = target;
    p[10] = 0xff;                // jmp
    p[11] = 0xe0;                // rax
}

// Font hook needs a custom trampoline because the original prologue contains a
// conditional branch ("test rdx,rdx / je ..." -> the NULL-text path).
static bool InstallFontHook(BYTE* target) {
    // Prologue is expected to be: 48 85 D2 / 0F 84 <rel32> / <reg-saves> /
    // 48 83 EC <imm>. Both the NULL-text branch and the register-save length
    // are decoded at runtime so a Toolbag update only needs a new signature.
    if (target[0] != 0x48 || target[1] != 0x85 || target[2] != 0xd2 ||
        target[3] != 0x0f || target[4] != 0x84)
        return false;

    const int32_t nullRel = *(int32_t*)(target + 5);
    BYTE* nullPath = target + 9 + nullRel;

    size_t subOff = 9;
    while (subOff + 3 < 64) {
        if (target[subOff] == 0x48 && target[subOff+1] == 0x83 &&
            target[subOff+2] == 0xec)
            break;
        subOff++;
    }
    if (subOff + 3 >= 64) return false;
    const size_t patchSize = subOff;
    if (patchSize < 12) return false;

    BYTE* trampoline = (BYTE*)VirtualAlloc(nullptr, 64,
                                           MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampoline) return false;

    size_t q = 0;
    trampoline[q++] = 0x48;                 // test rdx,rdx
    trampoline[q++] = 0x85;
    trampoline[q++] = 0xd2;
    trampoline[q++] = 0x75;                 // jnz +0x0c
    trampoline[q++] = 0x0c;
    trampoline[q++] = 0x48;                 // mov rax, <nullPath>
    trampoline[q++] = 0xb8;
    *(BYTE**)(trampoline + q) = nullPath; q += 8;
    trampoline[q++] = 0xff;                 // jmp rax
    trampoline[q++] = 0xe0;
    memcpy(trampoline + q, target + 9, patchSize - 9); q += (patchSize - 9);
    EmitJump(trampoline + q, target + patchSize);

    DWORD oldProtect;
    if (!VirtualProtect(target, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return false;
    }
    EmitJump(target, (void*)HandleFontCompile);
    for (size_t i = 12; i < patchSize; i++) target[i] = 0x90;
    VirtualProtect(target, patchSize, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), target, patchSize);

    g_originalFontCompile = (FontCompileFn)trampoline;
    return true;
}

// ---------------------------------------------------------------------------
// Installation
// ---------------------------------------------------------------------------
static BOOL CALLBACK InstallHooks(PINIT_ONCE, PVOID, PVOID*) {
    // Resolve log paths under %LOCALAPPDATA%\Marmoset Toolbag 5.
    char localAppData[MAX_PATH];
    const DWORD ln = GetEnvironmentVariableA("LOCALAPPDATA", localAppData, MAX_PATH);
    if (ln && ln < MAX_PATH) {
        const std::string dir = std::string(localAppData) + "\\Marmoset Toolbag 5";
        g_missingLogPath = dir + "\\ChineseLocalizer_missing.tsv";
        g_traceLogPath   = dir + "\\ChineseLocalizer_trace.tsv";
    }

    // Diagnostics: presence of trace.enabled next to the DLL turns on tracing.
    char modulePath[MAX_PATH] = {};
    const DWORD mn = GetModuleFileNameA(g_hookModule, modulePath, MAX_PATH);
    if (mn && mn < MAX_PATH) {
        char* slash = strrchr(modulePath, '\\');
        if (slash) {
            strcpy_s(slash + 1, MAX_PATH - (slash - modulePath + 1), "trace.enabled");
            g_traceEnabled = GetFileAttributesA(modulePath) != INVALID_FILE_ATTRIBUTES;
        }
    }

    // Also honor trace.enabled under %LOCALAPPDATA%\Marmoset Toolbag 5
    // (writable without admin) so the marker can be created easily.
    if (!g_traceEnabled) {
        char local[MAX_PATH];
        const DWORD ln = GetEnvironmentVariableA("LOCALAPPDATA", local, MAX_PATH);
        if (ln && ln < MAX_PATH) {
            char marker[MAX_PATH];
            sprintf_s(marker, "%s\\Marmoset Toolbag 5\\trace.enabled", local);
            g_traceEnabled = GetFileAttributesA(marker) != INVALID_FILE_ATTRIBUTES;
        }
    }

    if (!LoadDictionaries()) return TRUE;

    BYTE* imageBase = (BYTE*)GetModuleHandleW(nullptr);
    auto* dos = (IMAGE_DOS_HEADER*)imageBase;
    auto* nt  = (IMAGE_NT_HEADERS64*)(imageBase + dos->e_lfanew);
    g_imageBegin = imageBase;
    g_imageEnd   = imageBase + nt->OptionalHeader.SizeOfImage;

    // ----------------------------------------------------------------------
    // Self-adaptive hooking: each layer is located & installed independently.
    // If a newer Toolbag version changes one function's machine-code, only
    // that layer is skipped - the others still translate. (auto one-click)
    // ----------------------------------------------------------------------

    // Resolve the mset::Text vtable once (version-stable RTTI anchor).
    BYTE* textVtable = ResolveVtableByName(imageBase, ".?AVText@mset@@");

    // 1) Font::CompiledString - RTTI/vtable (Text slot[16] -> compile call).
    //    This is the only translation layer: it draws with Chinese while the
    //    engine's stored strings stay English, so command/callback keys remain
    //    intact (buttons work).
    if (textVtable) {
        BYTE* slot16 = (BYTE*)*(void**)(textVtable + 16 * sizeof(void*));
        if (slot16 && InstallFontHookViaVtable(slot16, g_imageBegin, g_imageEnd))
            g_hooksInstalled = TRUE;
    }

    // 4) GDI / USER32 capture safety-net (system APIs, version independent).
    InstallGdiCaptureHooks((HMODULE)imageBase);

    return TRUE;
}

// Exported entry point used by the launcher via a remote thread.
extern "C" __declspec(dllexport) BOOL WINAPI InstallHook() {
    InitOnceExecuteOnce(&g_initOnce, InstallHooks, nullptr, nullptr);
    return g_hooksInstalled;
}

// Worker thread that signals the launcher once hooks are installed.
static DWORD WINAPI InstallHookThread(LPVOID) {
    if (InstallHook()) {
        wchar_t eventName[96];
        swprintf_s(eventName, L"Local\\ToolbagChineseHookReady_%lu", GetCurrentProcessId());
        HANDLE readyEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName);
        if (readyEvent) {
            SetEvent(readyEvent);
            CloseHandle(readyEvent);
        }
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hookModule = module;
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, InstallHookThread, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
