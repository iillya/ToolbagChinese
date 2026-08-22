// ============================================================================
//  Toolbag 5 Chinese Localizer - Hook DLL
// ============================================================================
//  Runtime UI translation for Marmoset Toolbag 5.
//
//  It hooks the font compile and measurement paths so text is translated *in memory*
//  without touching Toolbag's assets/paths/index/configuration:
//    * Font::CompiledString     -> self-drawn text (Slug)
//    * Font measurement         -> Chinese glyph bounds during layout
//
//  Pressing F12 opens a short diagnostic capture window. Outside that window
//  the sniffer performs only an atomic branch and does no allocation or I/O.
//
//  Dictionary loaded next to this DLL:
//    * dictionary_zh.json  -> merged UI + asset translations (sp-translation-v1)
// ============================================================================

#include <Windows.h>
#include <Shellapi.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <vector>
#include <algorithm>
#include <utility>
#include <climits>
#include <cstdio>
#include <cstring>
#include <wchar.h>
#include <intrin.h>

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------
using FontCompileFn   = void(__fastcall*)(void*, const char*);
// Font text measurement.  The function writes width/height to the two output
// pointers.  The final two arguments are the engine's character-limit and
// single-line flags; keeping them in the hook is important because they are
// passed on the stack by the Windows x64 ABI.
using FontMeasureFn   = void(__fastcall*)(void*, float*, float*, const char*, int, bool);

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
static HMODULE          g_hookModule = nullptr;
static FontCompileFn    g_originalFontCompile[2] = {};
static FontMeasureFn    g_originalFontMeasure = nullptr;


static thread_local uintptr_t g_captureCallsite = 0;

struct TransparentStringHash {
    using is_transparent = void;
    size_t operator()(std::string_view value) const noexcept {
        return std::hash<std::string_view>{}(value);
    }
    size_t operator()(const std::string& value) const noexcept {
        return (*this)(std::string_view(value));
    }
};
struct TransparentStringEqual {
    using is_transparent = void;
    bool operator()(std::string_view left, std::string_view right) const noexcept {
        return left == right;
    }
};
using StringMap = std::unordered_map<std::string, std::string,
                                     TransparentStringHash, TransparentStringEqual>;
static StringMap        g_translations;
static StringMap        g_paddedTranslationCache;
static std::mutex       g_translationCacheMutex;

static std::unordered_set<std::string> g_capturedStrings;
static std::mutex       g_captureMutex;
static std::wstring     g_captureLogPath;
static std::atomic_bool g_captureActive{false};
static HWND             g_authorWindow = nullptr;
static HWND             g_gitHubWindow = nullptr;
static HWND             g_toolbagWindow = nullptr;
static int              g_authorX = INT_MIN;
static int              g_authorY = INT_MIN;
static int              g_authorWidth = 0;
static int              g_authorHeight = 0;
static int              g_gitHubX = INT_MIN;
static int              g_gitHubWidth = 0;
static int              g_menuBarHeight = 0;
static HWINEVENTHOOK    g_locationEventHook = nullptr;
static HWINEVENTHOOK    g_minimizeEventHook = nullptr;
static HWINEVENTHOOK    g_visibilityEventHook = nullptr;

static const wchar_t kAuthorText[] = L"Bilibili神说要凑数汉化";
static const wchar_t kAuthorUrl[] =
    L"https://space.bilibili.com/281243426?spm_id_from=333.1007.0.0";
static const wchar_t kGitHubText[] = L"Github仓库";
static const wchar_t kGitHubUrl[] = L"https://github.com/iillya/Toolbag";
static constexpr COLORREF kLinkTransparentColor = RGB(1, 2, 3);
static constexpr DWORD kLinkTransparentDibPixel = 0x00010203;

static bool RenderLinkWindow(HWND window, const wchar_t* text,
                             int x, int y, int width, int height);
static BOOL CALLBACK FindMainToolbagWindow(HWND window, LPARAM resultValue);

static std::wstring GetModulePath(HMODULE module) {
    std::vector<wchar_t> buffer(1024);
    while (buffer.size() <= 32768) {
        const DWORD length = GetModuleFileNameW(
            module, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (!length) return L"";
        if (length < buffer.size() - 1)
            return std::wstring(buffer.data(), length);
        buffer.resize(buffer.size() * 2);
    }
    return L"";
}

static INIT_ONCE        g_initOnce = INIT_ONCE_STATIC_INIT;
static BOOL             g_hooksInstalled = FALSE;

// ---------------------------------------------------------------------------
// Dictionary loading
// ---------------------------------------------------------------------------
// Minimal JSON string unescaper ("... \n \t \\ \" \uXXXX ...").
static std::string DecodeJsonEscapes(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];
        if (c != '\\' || i + 1 >= input.size()) { out += c; continue; }
        char e = input[++i];
        switch (e) {
            case 'n': out += '\n'; break;
            case 't': out += '\t'; break;
            case 'r': out += '\r'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'u': {
                if (i + 4 < input.size()) {
                    unsigned cp = 0;
                    for (int j = 1; j <= 4; j++) {
                        char h = input[i + j]; cp <<= 4;
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
static std::string ParseJsonString(const std::string& text, size_t& position) {
    std::string out;
    while (position < text.size()) {
        char c = text[position];
        if (c == '\\' && position + 1 < text.size()) {
            out += c; out += text[position + 1]; position += 2; continue;
        }
        if (c == '"') { position++; break; }
        out += c; position++;
    }
    return DecodeJsonEscapes(out);
}

// Load a sp-translation-v1 JSON dictionary (format like my_assets_zh.json):
// { ..., "translations": { "English": "中文", ... } }.
static bool LoadTranslationDictionary(const wchar_t* fileName) {
    std::wstring path = GetModulePath(g_hookModule);
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return false;
    path.replace(slash + 1, std::wstring::npos, fileName);

    FILE* file = nullptr;
    if (_wfopen_s(&file, path.c_str(), L"rb") || !file) return false;
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
        std::string key = ParseJsonString(text, pos);
        while (pos < text.size() && (text[pos]==' '||text[pos]=='\t'||text[pos]=='\r'||text[pos]=='\n')) pos++;
        if (pos < text.size() && text[pos] == ':') pos++;
        while (pos < text.size() && (text[pos]==' '||text[pos]=='\t'||text[pos]=='\r'||text[pos]=='\n')) pos++;
        if (pos >= text.size() || text[pos] != '"') break;
        pos++;                                        // opening quote of value
        std::string value = ParseJsonString(text, pos);
        if (!key.empty() && !value.empty()) {
            g_translations[key] = value;
            any = true;
        }
    }
    return any;
}

static bool LoadDictionary() {
    return LoadTranslationDictionary(L"dictionary_zh.json");
}

// Records untranslated, printable English UI strings once per process.
static void CaptureMissingString(const char* source, const char* text, size_t length) {
    if (!g_captureActive.load(std::memory_order_relaxed)) return;
    if (!text || length < 2 || length > 256) return;
    bool hasLetter = false;
    for (size_t i = 0; i < length; i++) {
        const unsigned char c = text[i];
        if (c == '/' || c == '\\' || c < 0x20 || c > 0x7e) return;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) hasLetter = true;
    }
    if (!hasLetter) return;

    const std::string value(text, length);
    if (g_translations.find(value) != g_translations.end()) return;
    const uintptr_t imageBase = (uintptr_t)GetModuleHandleW(nullptr);
    char row[768];
    sprintf_s(row, "%s\t0x%llX\t%s", source,
              (unsigned long long)(g_captureCallsite - imageBase), value.c_str());
    {
        std::lock_guard<std::mutex> lock(g_captureMutex);
        // Close the race with F12 ending while this string was being filtered.
        if (!g_captureActive.load(std::memory_order_relaxed)) return;
        if (g_capturedStrings.size() < 10000) g_capturedStrings.insert(row);
    }
}

// ---------------------------------------------------------------------------
// Translation core
// ---------------------------------------------------------------------------
// Some strings are sentinel values, not user-facing text to localize.
// Translating them (e.g. "None" -> "无") breaks controls that use them as a
// "no selection / NULL" marker (combo boxes drop the text / show blank).
// Translate 's' using the dictionary. Preserves surrounding whitespace and
// caches the result so the same string is not rebuilt repeatedly.
// SAFETY: returns a pointer into g_paddedTranslationCache. The map is
// node-based and entries are never erased/replaced, so returned pointers stay
// valid for the lifetime of the process (engine may hold them).
static const char* TranslateText(const char* text) {
    if (!text || !*text) return text;

    const std::string_view whole(text);
    const size_t first = whole.find_first_not_of(" \t");
    const size_t last  = whole.find_last_not_of(" \t");
    if (first == std::string_view::npos) return text;

    const std::string_view key = whole.substr(first, last - first + 1);
    if ((key.size() == 4 && _strnicmp(key.data(), "None", 4) == 0) ||
        (key.size() == 4 && _strnicmp(key.data(), "NULL", 4) == 0)) return text;
    const auto translation = g_translations.find(key);
    if (translation == g_translations.end()) {
        CaptureMissingString("FONT", key.data(), key.size());
        return text;
    }

    // The common case needs no allocation or lock. Dictionary nodes are never
    // modified after startup, so value pointers remain stable for the process.
    if (first == 0 && last + 1 == whole.size()) return translation->second.c_str();

    {
        std::lock_guard<std::mutex> lock(g_translationCacheMutex);
        const auto cached = g_paddedTranslationCache.find(whole);
        if (cached != g_paddedTranslationCache.end()) return cached->second.c_str();

        std::string wholeString(whole);
        std::string translated = wholeString.substr(0, first) + translation->second +
                                 wholeString.substr(last + 1);
        const auto inserted = g_paddedTranslationCache.emplace(
            std::move(wholeString), std::move(translated));
        return inserted.first->second.c_str();
    }
}

static const char* TranslateSafely(const char* text) {
    __try {
        return TranslateText(text);
    // Toolbag owns the incoming pointer. A stale engine pointer must fall back
    // to the original value instead of taking down the host process.
#pragma warning(suppress: 6320)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return text;
    }
}

// ---------------------------------------------------------------------------
// Engine hooks (replacement functions)
// ---------------------------------------------------------------------------
// Font::CompiledString - translate before the compiled result is built/measured.
static void InvokeTranslatedFontCompile(int hookIndex, void* instance,
                                        const char* text, uintptr_t callsite) {
    if (g_captureActive.load(std::memory_order_relaxed))
        g_captureCallsite = callsite;
    const char* translated = TranslateSafely(text);
    g_originalFontCompile[hookIndex](instance, translated);
}
static void HookPrimaryFontCompile(void* instance, const char* text) {
    InvokeTranslatedFontCompile(0, instance, text, (uintptr_t)_ReturnAddress());
}
static void HookSecondaryFontCompile(void* instance, const char* text) {
    InvokeTranslatedFontCompile(1, instance, text, (uintptr_t)_ReturnAddress());
}

// Measure exactly the same translated string that the compile/draw path sees.
// No widget-owned text is changed, so command names, enum values and callbacks
// remain English internally while layout receives the Chinese glyph bounds.
static void HookFontMeasure(void* instance, float* width, float* height,
                            const char* text, int characterLimit,
                            bool singleLine) {
    if (g_captureActive.load(std::memory_order_relaxed))
        g_captureCallsite = (uintptr_t)_ReturnAddress();
    const char* translated = TranslateSafely(text);
    g_originalFontMeasure(instance, width, height, translated,
                          characterLimit, singleLine);
}

static BOOL CALLBACK RedrawToolbagWindow(HWND window, LPARAM processIdValue) {
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId == (DWORD)processIdValue) {
        RedrawWindow(window, nullptr, nullptr,
                     RDW_INVALIDATE | RDW_ERASE | RDW_FRAME |
                     RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
    return TRUE;
}

static void RevealCaptureLog() {
    if (g_captureLogPath.empty() ||
        GetFileAttributesW(g_captureLogPath.c_str()) == INVALID_FILE_ATTRIBUTES)
        return;
    const std::wstring parameters = L"/select,\"" + g_captureLogPath + L"\"";
    ShellExecuteW(nullptr, L"open", L"explorer.exe", parameters.c_str(),
                  nullptr, SW_SHOWNORMAL);
}

static void PositionLinkWindows() {
    if (!g_authorWindow || !g_gitHubWindow) return;
    if (!IsWindow(g_toolbagWindow)) {
        g_toolbagWindow = nullptr;
        EnumWindows(FindMainToolbagWindow, (LPARAM)&g_toolbagWindow);
    }
    if (!g_toolbagWindow) return;
    if (!IsWindowVisible(g_toolbagWindow) || IsIconic(g_toolbagWindow)) {
        ShowWindow(g_authorWindow, SW_HIDE);
        ShowWindow(g_gitHubWindow, SW_HIDE);
        return;
    }
    RECT client = {};
    if (!GetClientRect(g_toolbagWindow, &client)) return;
    POINT origin = {0, 0};
    if (!ClientToScreen(g_toolbagWindow, &origin)) return;
    // Toolbag renders its custom chrome using its own UI scale, which is not
    // necessarily the Win32 window DPI. Measure the first solid title/menu
    // band from the composed screen pixels instead of guessing from DPI.
    if (g_menuBarHeight == 0) {
        HDC screenDc = GetDC(nullptr);
        if (screenDc) {
            const int sampleX = origin.x + client.right / 2;
            const COLORREF reference = GetPixel(screenDc, sampleX, origin.y + 2);
            int changedRows = 0;
            if (reference != CLR_INVALID) {
                for (int row = 3; row < 90; ++row) {
                    const COLORREF pixel = GetPixel(screenDc, sampleX, origin.y + row);
                    const int difference =
                        abs((int)GetRValue(pixel) - (int)GetRValue(reference)) +
                        abs((int)GetGValue(pixel) - (int)GetGValue(reference)) +
                        abs((int)GetBValue(pixel) - (int)GetBValue(reference));
                    changedRows = difference > 24 ? changedRows + 1 : 0;
                    if (changedRows == 3) {
                        const int measuredHeight = row - 2;
                        if (measuredHeight >= 30 && measuredHeight <= 70)
                            g_menuBarHeight = measuredHeight;
                        break;
                    }
                }
            }
            ReleaseDC(nullptr, screenDc);
        }
    }
    const int height = g_menuBarHeight ? g_menuBarHeight : 45;
    const int margin = MulDiv(8, height, 45);
    const int gap = MulDiv(14, height, 45);
    const int windowControlsWidth = MulDiv(150, height, 45);
    if (height != g_authorHeight || g_authorWidth == 0 || g_gitHubWidth == 0) {
        HDC dc = GetDC(nullptr);
        if (dc) {
            HFONT font = CreateFontW(-MulDiv(20, height, 45), 0, 0, 0,
                                     FW_NORMAL, FALSE, TRUE, FALSE, DEFAULT_CHARSET,
                                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     ANTIALIASED_QUALITY, DEFAULT_PITCH,
                                     L"Microsoft YaHei UI");
            HFONT oldFont = (HFONT)SelectObject(dc, font);
            SIZE size = {};
            if (GetTextExtentPoint32W(dc, kAuthorText, (int)wcslen(kAuthorText), &size))
                g_authorWidth = size.cx;
            if (GetTextExtentPoint32W(dc, kGitHubText, (int)wcslen(kGitHubText), &size))
                g_gitHubWidth = size.cx;
            SelectObject(dc, oldFont);
            DeleteObject(font);
            ReleaseDC(nullptr, dc);
        }
    }
    if (g_authorWidth <= 0) g_authorWidth = MulDiv(210, height, 45);
    if (g_gitHubWidth <= 0) g_gitHubWidth = MulDiv(100, height, 45);
    const int githubX = origin.x + client.right - windowControlsWidth -
                        g_gitHubWidth - margin;
    const int x = githubX - gap - g_authorWidth;
    const int y = origin.y;
    if (x == g_authorX && githubX == g_gitHubX && y == g_authorY &&
        height == g_authorHeight && IsWindowVisible(g_authorWindow) &&
        IsWindowVisible(g_gitHubWindow)) return;
    const bool needsRender = height != g_authorHeight ||
                             !IsWindowVisible(g_authorWindow) ||
                             !IsWindowVisible(g_gitHubWindow);
    g_authorX = x; g_gitHubX = githubX; g_authorY = y; g_authorHeight = height;
    if (needsRender) {
        RenderLinkWindow(g_authorWindow, kAuthorText, x, y, g_authorWidth, height);
        RenderLinkWindow(g_gitHubWindow, kGitHubText, githubX, y,
                         g_gitHubWidth, height);
    } else {
        SetWindowPos(g_authorWindow, HWND_TOP, x, y, g_authorWidth, height,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
        SetWindowPos(g_gitHubWindow, HWND_TOP, githubX, y, g_gitHubWidth, height,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

static int CalculateTextVerticalOffset(HDC referenceDc, HFONT font,
                                       const wchar_t* text,
                                       int width, int height) {
    if (width <= 0 || height <= 0) return 0;

    BITMAPINFO bitmapInfo = {};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    void* pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(referenceDc, &bitmapInfo, DIB_RGB_COLORS,
                                      &pixels, nullptr, 0);
    HDC memoryDc = CreateCompatibleDC(referenceDc);
    if (!bitmap || !memoryDc || !pixels) {
        if (memoryDc) DeleteDC(memoryDc);
        if (bitmap) DeleteObject(bitmap);
        return 0;
    }

    HBITMAP oldBitmap = (HBITMAP)SelectObject(memoryDc, bitmap);
    HFONT oldFont = (HFONT)SelectObject(memoryDc, font);
    RECT rect = {0, 0, width, height};
    HBRUSH background = CreateSolidBrush(kLinkTransparentColor);
    FillRect(memoryDc, &rect, background);
    DeleteObject(background);
    SetBkMode(memoryDc, TRANSPARENT);
    SetTextColor(memoryDc, RGB(0, 174, 236));
    DrawTextW(memoryDc, text, -1, &rect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // A 32-bit BI_RGB DIB stores bytes as B, G, R, 0, while COLORREF stores
    // them as R, G, B. Compare against the DIB's native DWORD layout.
    const DWORD* bitmapPixels = static_cast<const DWORD*>(pixels);
    int inkTop = height;
    int inkBottom = -1;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if ((bitmapPixels[y * width + x] & 0x00FFFFFF) !=
                kLinkTransparentDibPixel) {
                if (y < inkTop) inkTop = y;
                if (y > inkBottom) inkBottom = y;
            }
        }
    }

    SelectObject(memoryDc, oldFont);
    SelectObject(memoryDc, oldBitmap);
    DeleteDC(memoryDc);
    DeleteObject(bitmap);
    if (inkBottom < inkTop) return 0;

    // Align the center of the pixels that are actually drawn, including the
    // underline, with the exact center of Toolbag's 45-DIP menu bar.
    return (height - 1 - inkTop - inkBottom) / 2;
}

static bool RenderLinkWindow(HWND window, const wchar_t* text,
                             int x, int y, int width, int height) {
    HDC screenDc = GetDC(nullptr);
    HDC memoryDc = screenDc ? CreateCompatibleDC(screenDc) : nullptr;
    BITMAPINFO bitmapInfo = {};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    void* pixels = nullptr;
    HBITMAP bitmap = screenDc ? CreateDIBSection(screenDc, &bitmapInfo,
        DIB_RGB_COLORS, &pixels, nullptr, 0) : nullptr;
    if (!screenDc || !memoryDc || !bitmap || !pixels) {
        if (bitmap) DeleteObject(bitmap);
        if (memoryDc) DeleteDC(memoryDc);
        if (screenDc) ReleaseDC(nullptr, screenDc);
        return false;
    }

    HBITMAP oldBitmap = (HBITMAP)SelectObject(memoryDc, bitmap);
    memset(pixels, 0, (size_t)width * height * sizeof(DWORD));
    HFONT font = CreateFontW(-MulDiv(20, height, 45), 0, 0, 0,
                             FW_NORMAL, FALSE, TRUE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY, DEFAULT_PITCH,
                             L"Microsoft YaHei UI");
    HFONT oldFont = (HFONT)SelectObject(memoryDc, font);
    SetBkMode(memoryDc, TRANSPARENT);
    SetTextColor(memoryDc, RGB(0, 174, 236));
    RECT textRect = {0, 0, width, height};
    OffsetRect(&textRect, 0, CalculateTextVerticalOffset(
        memoryDc, font, text, width, height));
    DrawTextW(memoryDc, text, -1, &textRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    DWORD* bitmapPixels = static_cast<DWORD*>(pixels);
    for (size_t i = 0, count = (size_t)width * height; i < count; ++i) {
        // Alpha 1 is visually indistinguishable from transparent black but is
        // still hit-testable. Any rendered text pixel remains fully opaque.
        bitmapPixels[i] = (bitmapPixels[i] & 0x00FFFFFF)
            ? (bitmapPixels[i] | 0xFF000000)
            : 0x01000000;
    }

    POINT destination = {x, y};
    POINT source = {0, 0};
    SIZE size = {width, height};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    const BOOL updated = UpdateLayeredWindow(window, screenDc, &destination,
        &size, memoryDc, &source, 0, &blend, ULW_ALPHA);
    SelectObject(memoryDc, oldFont);
    SelectObject(memoryDc, oldBitmap);
    DeleteObject(font);
    DeleteObject(bitmap);
    DeleteDC(memoryDc);
    ReleaseDC(nullptr, screenDc);
    ShowWindow(window, SW_SHOWNOACTIVATE);
    return updated != FALSE;
}

static void CALLBACK LinkWindowEventCallback(HWINEVENTHOOK, DWORD, HWND window,
                                              LONG, LONG, DWORD, DWORD) {
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId == GetCurrentProcessId() && window != g_authorWindow &&
        window != g_gitHubWindow && g_authorWindow)
        PostMessageW(g_authorWindow, WM_APP + 1, 0, 0);
}

static LRESULT CALLBACK LinkWindowProc(HWND window, UINT message,
                                       WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_APP + 1:
            PositionLinkWindows();
            return 0;
        case WM_DISPLAYCHANGE:
        case WM_DPICHANGED:
            g_menuBarHeight = 0;
            PositionLinkWindows();
            return 0;
        case WM_SETCURSOR:
            SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32649)));
            return TRUE;
        case WM_LBUTTONUP:
            if (window == g_authorWindow)
                ShellExecuteW(nullptr, L"open", kAuthorUrl, nullptr, nullptr, SW_SHOWNORMAL);
            else if (window == g_gitHubWindow)
                ShellExecuteW(nullptr, L"open", kGitHubUrl, nullptr, nullptr, SW_SHOWNORMAL);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT paint = {};
            BeginPaint(window, &paint);
            EndPaint(window, &paint);
            return 0;
        }
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

static BOOL CALLBACK FindMainToolbagWindow(HWND window, LPARAM resultValue) {
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId != GetCurrentProcessId() || !IsWindowVisible(window) ||
        GetWindow(window, GW_OWNER) != nullptr) return TRUE;
    wchar_t title[256] = {};
    GetWindowTextW(window, title, _countof(title));
    if (!wcsstr(title, L"Toolbag")) return TRUE;
    *(HWND*)resultValue = window;
    return FALSE;
}

static DWORD WINAPI RunLinkWindowThread(LPVOID) {
    for (int i = 0; i < 300 && !g_toolbagWindow; i++) {
        EnumWindows(FindMainToolbagWindow, (LPARAM)&g_toolbagWindow);
        if (!g_toolbagWindow) Sleep(100);
    }
    if (!g_toolbagWindow) return 1;

    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = LinkWindowProc;
    windowClass.hInstance = g_hookModule;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32649));
    windowClass.lpszClassName = L"ToolbagChineseAuthorLink";
    RegisterClassW(&windowClass);
    // A normal child window is overdrawn by Toolbag's D3D swap chain. An owned,
    // non-activating popup is composed above D3D while still following Toolbag.
    g_authorWindow = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
        windowClass.lpszClassName, kAuthorText, WS_POPUP | WS_VISIBLE,
        0, 0, 1, 1, g_toolbagWindow, nullptr, g_hookModule, nullptr);
    if (!g_authorWindow) return 2;
    g_gitHubWindow = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
        windowClass.lpszClassName, kGitHubText, WS_POPUP | WS_VISIBLE,
        0, 0, 1, 1, g_toolbagWindow, nullptr, g_hookModule, nullptr);
    if (!g_gitHubWindow) {
        DestroyWindow(g_authorWindow);
        g_authorWindow = nullptr;
        return 3;
    }
    g_locationEventHook = SetWinEventHook(
        EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, nullptr,
        LinkWindowEventCallback, GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
    g_minimizeEventHook = SetWinEventHook(
        EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND, nullptr,
        LinkWindowEventCallback, GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
    g_visibilityEventHook = SetWinEventHook(
        EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE, nullptr, LinkWindowEventCallback,
        GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
    PositionLinkWindows();

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if (g_locationEventHook) UnhookWinEvent(g_locationEventHook);
    if (g_minimizeEventHook) UnhookWinEvent(g_minimizeEventHook);
    if (g_visibilityEventHook) UnhookWinEvent(g_visibilityEventHook);
    return 0;
}

static DWORD WINAPI MonitorCaptureHotkeyThread(LPVOID) {
    bool wasDown = false;
    while (true) {
        DWORD foregroundProcess = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
        const bool down = foregroundProcess == GetCurrentProcessId() &&
                          (GetAsyncKeyState(VK_F12) & 0x8000) != 0;
        if (down && !wasDown) {
            {
                std::lock_guard<std::mutex> lock(g_captureMutex);
                g_capturedStrings.clear();
            }
            g_captureActive.store(true, std::memory_order_release);
            EnumWindows(RedrawToolbagWindow, (LPARAM)GetCurrentProcessId());
            Sleep(1500);
            g_captureActive.store(false, std::memory_order_release);

            std::vector<std::string> rows;
            {
                std::lock_guard<std::mutex> lock(g_captureMutex);
                rows.assign(g_capturedStrings.begin(), g_capturedStrings.end());
                g_capturedStrings.clear();
            }
            std::sort(rows.begin(), rows.end());
            FILE* file = nullptr;
            bool saved = false;
            if (!g_captureLogPath.empty() &&
                !_wfopen_s(&file, g_captureLogPath.c_str(), L"wb") && file) {
                SYSTEMTIME now = {};
                GetLocalTime(&now);
                fprintf(file,
                    "{\r\n  \"captured_at\": \"%04u-%02u-%02uT%02u:%02u:%02u\","
                    "\r\n  \"duration_ms\": 1500,\r\n  \"entries\": [\r\n",
                    now.wYear, now.wMonth, now.wDay,
                    now.wHour, now.wMinute, now.wSecond);
                for (size_t i = 0; i < rows.size(); i++) {
                    const std::string& row = rows[i];
                    const size_t firstTab = row.find('\t');
                    const size_t secondTab = firstTab == std::string::npos
                        ? std::string::npos : row.find('\t', firstTab + 1);
                    const std::string source = row.substr(0, firstTab);
                    const std::string callsite = firstTab == std::string::npos ? "" :
                        row.substr(firstTab + 1, secondTab - firstTab - 1);
                    const std::string value = secondTab == std::string::npos ? "" :
                        row.substr(secondTab + 1);
                    auto writeJsonString = [file](const std::string& text) {
                        fputc('"', file);
                        for (unsigned char c : text) {
                            if (c == '"' || c == '\\') { fputc('\\', file); fputc(c, file); }
                            else if (c == '\n') fputs("\\n", file);
                            else if (c == '\r') fputs("\\r", file);
                            else if (c == '\t') fputs("\\t", file);
                            else fputc(c, file);
                        }
                        fputc('"', file);
                    };
                    fputs("    {\"source\": ", file); writeJsonString(source);
                    fputs(", \"callsite\": ", file); writeJsonString(callsite);
                    fputs(", \"text\": ", file); writeJsonString(value);
                    fputs(i + 1 < rows.size() ? "},\r\n" : "}\r\n", file);
                }
                fputs("  ]\r\n}\r\n", file);
                fclose(file);
                saved = true;
            }
            MessageBeep(saved ? MB_ICONASTERISK : MB_ICONHAND);
            if (saved) RevealCaptureLog();
        }
        wasDown = down;
        Sleep(50);
    }
}

// ---------------------------------------------------------------------------
// RTTI / vtable based locating (version-stable primary method).
// Locates functions from the RTTI type-name + vtable instead of byte patterns,
// so a Toolbag update that reorders/relocates code does not break the hooks.
// ---------------------------------------------------------------------------
static DWORD FindBytesInImage(BYTE* imageBase, const void* pattern,
                              size_t length, DWORD startRva = 0) {
    auto* dos = (IMAGE_DOS_HEADER*)imageBase;
    auto* nt  = (IMAGE_NT_HEADERS64*)(imageBase + dos->e_lfanew);
    const size_t size = nt->OptionalHeader.SizeOfImage;
    auto* bytes = static_cast<const BYTE*>(pattern);
    for (size_t rva = startRva; rva + length <= size; ++rva)
        if (!memcmp(imageBase + rva, bytes, length)) return (DWORD)rva;
    return 0;
}
static DWORD FindU32InImage(BYTE* imageBase, DWORD value, DWORD startRva = 0) {
    return FindBytesInImage(imageBase, &value, sizeof(value), startRva);
}
static DWORD FindU64InImage(BYTE* imageBase, uintptr_t value, DWORD startRva = 0) {
    return FindBytesInImage(imageBase, &value, sizeof(value), startRva);
}

// Resolve a class' primary vftable from its RTTI mangled type name.
static BYTE* ResolveVtableByName(BYTE* imageBase, const char* rttiName) {
    const DWORD nameRva = FindBytesInImage(
        imageBase, rttiName, (DWORD)strlen(rttiName) + 1);
    if (!nameRva) return nullptr;
    const DWORD typeDescriptorRva = nameRva - 0x10;
    DWORD searchRva = 0;
    while (true) {
        const DWORD referenceRva = FindU32InImage(
            imageBase, typeDescriptorRva, searchRva);
        if (!referenceRva) break;
        searchRva = referenceRva + 1;
        if (referenceRva < 12) continue;
        const DWORD locatorRva = referenceRva - 12;
        BYTE* locator = imageBase + locatorRva;
        const DWORD signature = *(DWORD*)(locator + 0x00);
        const DWORD referencedTypeDescriptor = *(DWORD*)(locator + 0x0C);
        if (signature > 1 || referencedTypeDescriptor != typeDescriptorRva)
            continue;
        const uintptr_t locatorAddress = (uintptr_t)imageBase + locatorRva;
        const DWORD vtableReferenceRva = FindU64InImage(imageBase, locatorAddress);
        if (vtableReferenceRva) return imageBase + vtableReferenceRva + 8;
    }
    return nullptr;
}

static void EmitJump(BYTE* p, const void* target);

static bool InstallFontHook(BYTE* target, int hookIndex);
static bool InstallMeasureHook(BYTE* target);
// Scan a function body for calls to "compile-string" shaped functions
// (test rdx,rdx ; je rel32 ...) and install the Font hook on the first that
// accepts our trampoline. Returns true on success.
static bool InstallFontHooksFromTextDraw(BYTE* textDrawMethod,
                                         BYTE* imageBegin, BYTE* imageEnd) {
    bool any = false;
    BYTE* installedTargets[2] = {};
    int installedCount = 0;
    BYTE* end = textDrawMethod + 0x400; if (end > imageEnd) end = imageEnd;
    for (BYTE* p = textDrawMethod; p + 5 <= end && installedCount < 2; p++) {
        if (p[0] != 0xE8) continue;
        const int32_t rel = *(int32_t*)(p + 1);
        BYTE* target = p + 5 + rel;
        if (target >= imageBegin && target < imageEnd &&
            target[0] == 0x48 && target[1] == 0x85 && target[2] == 0xd2 &&
            target[3] == 0x0f && target[4] == 0x84) {
            bool duplicate = false;
            for (int i = 0; i < installedCount; i++)
                if (installedTargets[i] == target) duplicate = true;
            if (!duplicate && InstallFontHook(target, installedCount)) {
                installedTargets[installedCount++] = target;
                any = true;
            }
        }
    }
    return any;
}

// Locate the font measurement routine from calls made by Text::draw.  The
// routine is identified by its stable semantic prologue: save non-volatiles,
// allocate a large shaping workspace, preserve R9 (the UTF-8 text), then test
// it for null.  This keeps the locator independent of the image base/RVA.
static bool InstallMeasureHookFromTextDraw(BYTE* textDrawMethod,
                                           BYTE* imageBegin, BYTE* imageEnd) {
    BYTE* end = textDrawMethod + 0x500; if (end > imageEnd) end = imageEnd;
    for (BYTE* p = textDrawMethod; p + 5 <= end; p++) {
        if (p[0] != 0xE8) continue;
        const int32_t rel = *(int32_t*)(p + 1);
        BYTE* target = p + 5 + rel;
        if (target < imageBegin || target + 0x40 >= imageEnd) continue;

        // mov [rsp+18],rbx; push rbp,rdi,r12,r14,r15; lea rbp,[rsp-...]
        static const BYTE prefix[] = {
            0x48,0x89,0x5c,0x24,0x18,0x55,0x57,0x41,0x54,0x41,0x56,0x41,0x57,
            0x48,0x8d,0xac,0x24
        };
        if (memcmp(target, prefix, sizeof(prefix)) != 0) continue;

        // The text argument is preserved from R9 and checked before shaping.
        bool savesText = false, testsText = false;
        for (size_t i = sizeof(prefix); i + 2 < 0x40; i++) {
            if (target[i] == 0x4d && target[i+1] == 0x8b && target[i+2] == 0xf9)
                savesText = true; // mov r15,r9
            if (target[i] == 0x4d && target[i+1] == 0x85 && target[i+2] == 0xc9)
                testsText = true; // test r9,r9
        }
        const bool hasTextShape = savesText && testsText;
        if (hasTextShape) return InstallMeasureHook(target);
    }
    return false;
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
static bool InstallFontHook(BYTE* target, int hookIndex) {
    if (hookIndex < 0 || hookIndex >= 2 || g_originalFontCompile[hookIndex])
        return false;
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

    size_t trampolineOffset = 0;
    trampoline[trampolineOffset++] = 0x48;                 // test rdx,rdx
    trampoline[trampolineOffset++] = 0x85;
    trampoline[trampolineOffset++] = 0xd2;
    trampoline[trampolineOffset++] = 0x75;                 // jnz +0x0c
    trampoline[trampolineOffset++] = 0x0c;
    trampoline[trampolineOffset++] = 0x48;                 // mov rax, <nullPath>
    trampoline[trampolineOffset++] = 0xb8;
    *(BYTE**)(trampoline + trampolineOffset) = nullPath;
    trampolineOffset += 8;
    trampoline[trampolineOffset++] = 0xff;                 // jmp rax
    trampoline[trampolineOffset++] = 0xe0;
    memcpy(trampoline + trampolineOffset, target + 9, patchSize - 9);
    trampolineOffset += patchSize - 9;
    EmitJump(trampoline + trampolineOffset, target + patchSize);

    DWORD oldProtect;
    if (!VirtualProtect(target, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return false;
    }
    EmitJump(target, hookIndex == 0 ? (void*)HookPrimaryFontCompile
                                    : (void*)HookSecondaryFontCompile);
    for (size_t i = 12; i < patchSize; i++) target[i] = 0x90;
    VirtualProtect(target, patchSize, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), target, patchSize);

    g_originalFontCompile[hookIndex] = (FontCompileFn)trampoline;
    return true;
}

// The measurement prologue's first 13 bytes contain whole instructions and no
// RIP-relative operands, so they can be copied verbatim into a trampoline.
static bool InstallMeasureHook(BYTE* target) {
    if (g_originalFontMeasure) return true;

    static const BYTE prefix[] = {
        0x48,0x89,0x5c,0x24,0x18,0x55,0x57,0x41,0x54,0x41,0x56,0x41,0x57
    };
    if (memcmp(target, prefix, sizeof(prefix)) != 0) return false;

    constexpr size_t patchSize = sizeof(prefix);
    BYTE* trampoline = (BYTE*)VirtualAlloc(nullptr, 64,
                                           MEM_COMMIT | MEM_RESERVE,
                                           PAGE_EXECUTE_READWRITE);
    if (!trampoline) return false;
    memcpy(trampoline, target, patchSize);
    EmitJump(trampoline + patchSize, target + patchSize);

    DWORD oldProtect;
    if (!VirtualProtect(target, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return false;
    }
    EmitJump(target, (void*)HookFontMeasure);
    for (size_t i = 12; i < patchSize; i++) target[i] = 0x90;
    VirtualProtect(target, patchSize, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), target, patchSize);

    g_originalFontMeasure = (FontMeasureFn)trampoline;
    return true;
}

// ---------------------------------------------------------------------------
// Installation
// ---------------------------------------------------------------------------
static BOOL CALLBACK InitializeHooks(PINIT_ONCE, PVOID, PVOID*) {
    std::wstring logPath = GetModulePath(g_hookModule);
    const size_t slash = logPath.find_last_of(L"\\/");
    if (slash != std::wstring::npos) {
        logPath.replace(slash + 1, std::wstring::npos,
                        L"ChineseLocalizer_sniffer.json");
        g_captureLogPath = std::move(logPath);
    }

    if (!LoadDictionary()) return TRUE;

    BYTE* imageBase = (BYTE*)GetModuleHandleW(nullptr);
    auto* dos = (IMAGE_DOS_HEADER*)imageBase;
    auto* nt  = (IMAGE_NT_HEADERS64*)(imageBase + dos->e_lfanew);
    BYTE* imageEnd = imageBase + nt->OptionalHeader.SizeOfImage;

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
        BYTE* textDrawMethod = (BYTE*)*(void**)(textVtable + 16 * sizeof(void*));
        if (textDrawMethod) {
            const bool compileReady =
                InstallFontHooksFromTextDraw(textDrawMethod, imageBase, imageEnd);
            const bool measureReady =
                InstallMeasureHookFromTextDraw(textDrawMethod, imageBase, imageEnd);
            // Drawing remains the required base layer.  Measurement improves
            // layout when available, but a version mismatch must not prevent
            // the safe display-only translation from starting.
            if (compileReady) g_hooksInstalled = TRUE;
            (void)measureReady;
        }
    }

    HANDLE monitor = CreateThread(nullptr, 0, MonitorCaptureHotkeyThread,
                                  nullptr, 0, nullptr);
    if (monitor) CloseHandle(monitor);

    HANDLE linkWindows = CreateThread(nullptr, 0, RunLinkWindowThread,
                                      nullptr, 0, nullptr);
    if (linkWindows) CloseHandle(linkWindows);

    return TRUE;
}

// Exported entry point used by the launcher via a remote thread.
extern "C" __declspec(dllexport) BOOL WINAPI InstallHook() {
    InitOnceExecuteOnce(&g_initOnce, InitializeHooks, nullptr, nullptr);
    return g_hooksInstalled;
}

// Worker thread that signals the launcher once hooks are installed.
static DWORD WINAPI InitializeHooksThread(LPVOID) {
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
        HANDLE thread = CreateThread(nullptr, 0, InitializeHooksThread,
                                     nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
