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
//  Dictionaries loaded next to this DLL:
//    * dictionary.txt           -> UI strings
//    * dictionary_assets.txt    -> asset / library names (kept separate)
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
using TextSetterFn    = void*(__fastcall*)(void*, const char*);
using StringAssignFn  = void*(__fastcall*)(void*, const char*, size_t);
using FontCompileFn   = void(__fastcall*)(void*, const char*);

// A masked byte used to locate a machine-code signature in the .text section.
struct BytePattern { BYTE value; BYTE mask; };

// ---------------------------------------------------------------------------
// Machine-code signatures (version-specific - DO NOT alter these bytes)
// ---------------------------------------------------------------------------
#define OP(a) { a, 0xff }   // exact byte
#define ANY   { 0, 0x00 }   // wildcard byte

// mset::Text::setText(const char*) prologue. The string-member offset is
// decoded from the ADD RCX, imm32 instruction so it is not hard-coded.
static const BytePattern kTextSetterPattern[] = {
    OP(0x48),OP(0x89),OP(0x5c),OP(0x24),OP(0x10),OP(0x48),OP(0x89),OP(0x6c),OP(0x24),OP(0x18),
    OP(0x56),OP(0x57),OP(0x41),OP(0x56),OP(0x48),OP(0x83),OP(0xec),OP(0x30),OP(0x4c),OP(0x8b),
    OP(0x71),OP(0x18),OP(0x49),OP(0x8b),OP(0xf0),OP(0x48),OP(0x8b),OP(0xea)
};

// Text setter pair used to find both Text::setText variants.
static const BytePattern kTextSetterFinderPattern[] = {
    OP(0x48),OP(0x85),OP(0xd2),OP(0x74),ANY,OP(0x49),OP(0xc7),OP(0xc0),OP(0xff),OP(0xff),OP(0xff),
    OP(0xff),OP(0x0f),OP(0x1f),OP(0x40),OP(0x00),OP(0x49),OP(0xff),OP(0xc0),OP(0x42),OP(0x80),
    OP(0x3c),OP(0x02),OP(0x00),OP(0x75),ANY,OP(0x48),OP(0x81),OP(0xc1),ANY,ANY,ANY,ANY
};

// Menu::MenuItem label copy followed by its separate callback/binding copy.
static const BytePattern kMenuItemPattern[] = {
    OP(0x48),OP(0x8d),OP(0x4f),OP(0x10),OP(0xe8),ANY,ANY,ANY,ANY,OP(0x48),OP(0x8b),OP(0xd5),
    OP(0x48),OP(0x8d),OP(0x4f),OP(0x38),OP(0xe8),ANY,ANY,ANY,ANY
};

// Font compiled-string cache entry; both layout measurement and Slug drawing
// consume the result produced by this function.
static const BytePattern kFontCompilePattern[] = {
    OP(0x48),OP(0x85),OP(0xd2),OP(0x0f),OP(0x84),ANY,ANY,ANY,ANY,OP(0x48),OP(0x89),OP(0x5c),
    OP(0x24),OP(0x08),OP(0x48),OP(0x89),OP(0x6c),OP(0x24),OP(0x18),OP(0x56),OP(0x57),OP(0x41),
    OP(0x56),OP(0x48),OP(0x83),OP(0xec),OP(0x30),OP(0x48),OP(0x8b),OP(0xf2),OP(0x4c),OP(0x8b),
    OP(0xf1),OP(0x80),OP(0x3a),OP(0x00),OP(0x0f),OP(0x84),ANY,ANY,ANY,ANY
};

#undef OP
#undef ANY

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
static HMODULE          g_hookModule = nullptr;
static StringAssignFn   g_originalStringAssign = nullptr;
static FontCompileFn    g_originalFontCompile = nullptr;

static size_t           g_textStringOffset1 = 0;
static size_t           g_textStringOffset2 = 0;
static uintptr_t        g_menuLabelAssignSite = 0;

static BYTE*            g_imageBegin = nullptr;
static BYTE*            g_imageEnd = nullptr;
static thread_local bool    g_assignBusy = false;
static thread_local uintptr_t g_callsite = 0;

static std::unordered_map<std::string,std::string> g_translationDict;
static std::unordered_map<std::string,std::string> g_displayCache;
static std::mutex       g_cacheLock;

static std::unordered_set<std::string> g_loggedMissing;
static std::mutex       g_missingLock;
static std::unordered_set<std::string> g_tracedCalls;

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
static bool LoadDictionaryFile(const char* fileName) {
    char path[MAX_PATH];
    const DWORD len = GetModuleFileNameA(g_hookModule, path, MAX_PATH);
    if (!len || len >= MAX_PATH) return false;

    char* slash = strrchr(path, '\\');
    if (!slash) return false;
    strcpy_s(slash + 1, MAX_PATH - (slash - path + 1), fileName);

    FILE* file = nullptr;
    if (fopen_s(&file, path, "r") || !file) return false;

    char line[2048];
    bool any = false;
    while (fgets(line, sizeof line, file)) {
        std::string s = Trim(line);
        if (s.empty() || s[0] == '#') continue;

        size_t sep = s.find(';');
        if (sep == std::string::npos) sep = s.find('\t');
        if (sep == std::string::npos) continue;

        std::string key = Trim(s.substr(0, sep));
        std::string val = Trim(s.substr(sep + 1));
        if (!key.empty() && !val.empty()) {
            g_translationDict[key] = val;
            any = true;
        }
    }
    fclose(file);
    return any;
}

static bool LoadDictionaries() {
    const bool uiLoaded    = LoadDictionaryFile("dictionary.txt");
    const bool assetsLoaded = LoadDictionaryFile("dictionary_assets.txt");
    return uiLoaded || assetsLoaded;
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

static bool IsTraceableString(const char* text, size_t length) {
    if (!text || length < 2 || length > 256) return false;
    bool hasLetter = false;
    for (size_t i = 0; i < length; i++) {
        const unsigned char c = text[i];
        if (c < 0x20 || c > 0x7e) return false;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) hasLetter = true;
    }
    return hasLetter;
}

static void TraceStringAssign(const char* text, size_t length, uintptr_t site,
                              bool imageSource, bool known, bool translated) {
    if (!g_traceEnabled || !IsTraceableString(text, length)) return;

    void* frames[12] = {};
    const USHORT count = CaptureStackBackTrace(0, _countof(frames), frames, nullptr);
    const uintptr_t imageBase = (uintptr_t)GetModuleHandleW(nullptr);

    char id[96];
    sprintf_s(id, "%llX:%u:%.*s", (unsigned long long)(site - imageBase),
              (unsigned)length, (int)(length > 48 ? 48 : length), text);

    {
        std::lock_guard<std::mutex> lock(g_missingLock);
        if (!g_tracedCalls.insert(id).second) return;
    }

    FILE* file = nullptr;
    if (fopen_s(&file, g_traceLogPath.c_str(), "ab") || !file) return;
    fprintf(file, "ASSIGN\t0x%llX\t%lu\t%s\t%s\t%s\t",
            (unsigned long long)(site - imageBase), GetCurrentThreadId(),
            imageSource ? "image" : "heap",
            known ? "known" : "missing",
            translated ? "translated" : "untouched");
    for (USHORT i = 0; i < count; i++) {
        const uintptr_t p = (uintptr_t)frames[i];
        if (i) fputc(',', file);
        if (p >= imageBase && p < (uintptr_t)g_imageEnd)
            fprintf(file, "+0x%llX", (unsigned long long)(p - imageBase));
        else
            fprintf(file, "@0x%llX", (unsigned long long)p);
    }
    fprintf(file, "\t%.*s\r\n", (int)length, text);
    fclose(file);
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
// Text::setText(char*)-style assign with a length. Handles menu labels only at
// the dedicated site; otherwise captures/traces but leaves text unchanged.
static void* HandleStringAssign(void* self, const char* text, size_t length) {
    if (g_assignBusy || !text || length == 0 || length > 256)
        return g_originalStringAssign(self, text, length);

    g_callsite = (uintptr_t)_ReturnAddress();
    const bool imageSource = (BYTE*)text >= g_imageBegin && (BYTE*)text + length < g_imageEnd;

    g_assignBusy = true;
    const std::string key(text, length);
    const auto it = g_translationDict.find(key);

    const char* out = text;
    size_t outLen = length;
    bool translated = false;

    // Only translate exact menu-label assigns (the dedicated site). Translating
    // every assign breaks engine startup, so keep this path conservative.
    // Sentinels (None/NULL) stay as-is.
    if (g_callsite == g_menuLabelAssignSite && it != g_translationDict.end() &&
        !IsSentinelText(key.c_str())) {
        out = it->second.c_str();
        outLen = it->second.size();
        translated = true;
    }

    TraceStringAssign(text, length, g_callsite, imageSource,
                      it != g_translationDict.end(), translated);
    g_assignBusy = false;

    return g_originalStringAssign(self, out, outLen);
}

// Lightweight per-hook trace (enabled by trace.enabled next to the DLL).
static void TraceHook(const char* kind, const char* text) {
    if (!g_traceEnabled || !text) return;
    FILE* f = nullptr;
    if (fopen_s(&f, g_traceLogPath.c_str(), "ab") || !f) return;
    fprintf(f, "HOOK\t%s\t%s\tlen=%zu\n", kind, text, strlen(text));
    fclose(f);
}

// Text::setText(const char*) - variant A: writes at +g_textStringOffset1.
static void* HandleTextSetter1(void* self, const char* text) {
    if (!text) return self;
    g_callsite = (uintptr_t)_ReturnAddress();
    TraceHook("TEXT1", text);
    const char* out = Translate(text);
    TraceHook("TEXT1->", out);
    return g_originalStringAssign((BYTE*)self + g_textStringOffset1, out, strlen(out));
}

// Text::setText(const char*) - variant B: writes at +g_textStringOffset2.
static void* HandleTextSetter2(void* self, const char* text) {
    if (!text) return self;
    g_callsite = (uintptr_t)_ReturnAddress();
    TraceHook("TEXT2", text);
    const char* out = Translate(text);
    TraceHook("TEXT2->", out);
    return g_originalStringAssign((BYTE*)self + g_textStringOffset2, out, strlen(out));
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
    if (!text || length == 0 || length > 256) return;
    char buf[512];
    const int len = WideCharToMultiByte(CP_UTF8, 0, text, (int)length,
                                        buf, (int)sizeof(buf) - 1, nullptr, nullptr);
    if (len <= 0 || len > 256) return;
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
static BOOL WINAPI HookSetWindowTextW(HWND hwnd,LPCWSTR text) {
    g_callsite = (uintptr_t)_ReturnAddress();
    LogGdiString(text, wcslen(text));
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
// Low-level hooking machinery
// ---------------------------------------------------------------------------
static bool FindTextSection(BYTE*& begin, size_t& size) {
    BYTE* base = (BYTE*)GetModuleHandleW(nullptr);
    if (!base) return false;

    auto* dos = (IMAGE_DOS_HEADER*)base;
    auto* nt  = (IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
    auto* sec = IMAGE_FIRST_SECTION(nt);

    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
        if (!memcmp(sec->Name, ".text", 5)) {
            begin = base + sec->VirtualAddress;
            size  = sec->Misc.VirtualSize;
            return true;
        }
    }
    return false;
}

// Search for a unique signature in .text; returns nullptr if not exactly one hit.
static BYTE* FindUniqueSignature(const BytePattern* pattern, size_t patternSize) {
    BYTE* section;
    size_t sectionSize;
    if (!FindTextSection(section, sectionSize) || sectionSize < patternSize)
        return nullptr;

    BYTE* found = nullptr;
    for (size_t i = 0; i <= sectionSize - patternSize; i++) {
        bool match = true;
        for (size_t j = 0; j < patternSize; j++) {
            if (pattern[j].mask && (section[i + j] & pattern[j].mask) != pattern[j].value) {
                match = false;
                break;
            }
        }
        if (match) {
            if (found) return nullptr; // more than one hit -> ambiguous
            found = section + i;
        }
    }
    return found;
}

// Find the two Text::setText variants; decodes and orders their string offsets.
static bool FindTextSetterPair(BYTE*& first, BYTE*& second) {
    BYTE* section;
    size_t sectionSize;
    if (!FindTextSection(section, sectionSize) ||
        sectionSize < _countof(kTextSetterFinderPattern)) {
        return false;
    }

    first = second = nullptr;
    for (size_t i = 0; i <= sectionSize - _countof(kTextSetterFinderPattern); i++) {
        bool match = true;
        for (size_t j = 0; j < _countof(kTextSetterFinderPattern); j++) {
            const BytePattern& p = kTextSetterFinderPattern[j];
            if (p.mask && (section[i + j] & p.mask) != p.value) { match = false; break; }
        }
        if (match) {
            if (!first)      first = section + i;
            else if (!second) second = section + i;
            else return false; // unexpected third hit
        }
    }
    if (!first || !second) return false;

    const DWORD off1 = *(DWORD*)(first + 29);
    const DWORD off2 = *(DWORD*)(second + 29);
    if (off1 == off2) return false;
    if (off1 > off2) { BYTE* t = first; first = second; second = t; }
    return true;
}

// Emit an absolute jump (mov rax, addr; jmp rax) at 'p' targeting 'target'.
static void EmitJump(BYTE* p, const void* target) {
    p[0] = 0x48;                 // REX.W
    p[1] = 0xb8;                 // mov rax, imm64
    *(const void**)(p + 2) = target;
    p[10] = 0xff;                // jmp
    p[11] = 0xe0;                // rax
}

// Create a trampoline that runs the original bytes then jumps back, and patch
// the target to jump to our hook. Returns the trampoline as the "original".
static bool InstallTrampolineHook(BYTE* target, size_t patchSize, const void* hook,
                                  TextSetterFn& trampolineOut) {
    BYTE* trampoline = (BYTE*)VirtualAlloc(nullptr, patchSize + 16,
                                           MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampoline) return false;

    memcpy(trampoline, target, patchSize);
    EmitJump(trampoline + patchSize, target + patchSize);

    DWORD oldProtect;
    if (!VirtualProtect(target, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return false;
    }

    EmitJump(target, hook);
    for (size_t i = 12; i < patchSize; i++) target[i] = 0x90; // NOP padding
    VirtualProtect(target, patchSize, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), target, patchSize);

    trampolineOut = (TextSetterFn)trampoline;
    return true;
}

// Patch the target to jump directly to our hook (no trampoline needed).
static bool PatchDirect(BYTE* target, size_t patchSize, const void* hook) {
    DWORD oldProtect;
    if (!VirtualProtect(target, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    EmitJump(target, hook);
    for (size_t i = 12; i < patchSize; i++) target[i] = 0x90;
    VirtualProtect(target, patchSize, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), target, patchSize);
    return true;
}

// Font hook needs a custom trampoline because the original prologue contains a
// conditional branch (the "test rdx,rdx / je ..." path at offset 0x164).
static bool InstallFontHook(BYTE* target) {
    BYTE* trampoline = (BYTE*)VirtualAlloc(nullptr, 64,
                                           MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampoline) return false;

    size_t q = 0;
    trampoline[q++] = 0x48;                 // test rdx,rdx
    trampoline[q++] = 0x85;
    trampoline[q++] = 0xd2;
    trampoline[q++] = 0x75;                 // jnz +0x0c
    trampoline[q++] = 0x0c;
    trampoline[q++] = 0x48;                 // mov rax, <original+0x164>
    trampoline[q++] = 0xb8;
    *(BYTE**)(trampoline + q) = target + 0x164; q += 8;
    trampoline[q++] = 0xff;                 // jmp rax
    trampoline[q++] = 0xe0;
    memcpy(trampoline + q, target + 9, 14); q += 14;
    EmitJump(trampoline + q, target + 23);

    DWORD oldProtect;
    if (!VirtualProtect(target, 23, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return false;
    }
    EmitJump(target, (void*)HandleFontCompile);
    for (size_t i = 12; i < 23; i++) target[i] = 0x90;
    VirtualProtect(target, 23, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), target, 23);

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

    // 1) Text::setText (main assign) + menu labels.
    if (BYTE* assignSite = FindUniqueSignature(kTextSetterPattern, _countof(kTextSetterPattern))) {
        TextSetterFn trampoline = nullptr;
        if (InstallTrampolineHook(assignSite, 18, (void*)HandleStringAssign, trampoline)) {
            g_originalStringAssign = (StringAssignFn)trampoline;
            g_hooksInstalled = TRUE;
        }
        // menu label assignment site (only used by HandleStringAssign above)
        if (BYTE* menuSite = FindUniqueSignature(kMenuItemPattern, _countof(kMenuItemPattern)))
            g_menuLabelAssignSite = (uintptr_t)(menuSite + 9);
    }

    // 2) Text::setText variants (require the assign trampoline above).
    if (g_originalStringAssign) {
        BYTE* s1 = nullptr;
        BYTE* s2 = nullptr;
        if (FindTextSetterPair(s1, s2)) {
            g_textStringOffset1 = *(DWORD*)(s1 + 29);
            g_textStringOffset2 = *(DWORD*)(s2 + 29);
            if (g_textStringOffset1 >= 0x20 && g_textStringOffset2 <= 0x1000) {
                const bool ok1 = PatchDirect(s1, 12, (void*)HandleTextSetter1);
                const bool ok2 = PatchDirect(s2, 12, (void*)HandleTextSetter2);
                if (ok1 || ok2) g_hooksInstalled = TRUE;
            }
        }
    }

    // 3) Font::CompiledString (independent layer).
    if (BYTE* fontSite = FindUniqueSignature(kFontCompilePattern, _countof(kFontCompilePattern))) {
        if (InstallFontHook(fontSite)) g_hooksInstalled = TRUE;
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
