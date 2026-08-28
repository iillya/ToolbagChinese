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
#include <vector>
#include <algorithm>
#include <utility>
#include <climits>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <wchar.h>
#include "Zydis.h"

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------
using FontCompileFn   = void(__fastcall*)(void*, const char*);
// Font text measurement.  The function writes width/height to the two output
// pointers.  The final two arguments are the engine's character-limit and
// single-line flags; keeping them in the hook is important because they are
// passed on the stack by the Windows x64 ABI.
using FontMeasureFn   = void(__fastcall*)(void*, float*, float*, const char*, int, bool);
using TitleBarLayoutFn = void(__fastcall*)(void*, void*);

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
static HMODULE          g_hookModule = nullptr;
static FontCompileFn    g_originalFontCompile[2] = {};
static FontMeasureFn    g_originalFontMeasure = nullptr;
static TitleBarLayoutFn g_originalTitleBarLayout = nullptr;
static BYTE*            g_windowVtable = nullptr;
static BYTE*            g_buttonVtable = nullptr;

struct TitleBarSnapshot {
    bool valid = false;
    float titleBarLayout[8] = {};
    float containerLayout[8] = {};
    float minimizeButtonLayout[8] = {};
};
static TitleBarSnapshot g_titleBarSnapshot = {};
static SRWLOCK g_titleBarSnapshotLock = SRWLOCK_INIT;


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

static HWND             g_authorWindow = nullptr;
static HWND             g_gitHubWindow = nullptr;
static HWND             g_toolbagWindow = nullptr;
static DWORD            g_toolbagMainThreadId = 0;
static int              g_authorX = INT_MIN;
static int              g_authorY = INT_MIN;
static int              g_authorWidth = 0;
static int              g_authorHeight = 0;
static int              g_gitHubX = INT_MIN;
static int              g_gitHubWidth = 0;
static HWINEVENTHOOK    g_locationEventHook = nullptr;
static HWINEVENTHOOK    g_minimizeEventHook = nullptr;
static HWINEVENTHOOK    g_visibilityEventHook = nullptr;

static const wchar_t kAuthorText[] = L"Bilibili神说要凑数汉化";
static const wchar_t kAuthorUrl[] =
    L"https://space.bilibili.com/281243426?spm_id_from=333.1007.0.0";
static const wchar_t kGitHubText[] = L"Github仓库";
static const wchar_t kGitHubUrl[] = L"https://github.com/iillya/Toolbag";
static constexpr COLORREF kLinkTextColor = RGB(102, 170, 255); // #66AAFF
static constexpr COLORREF kLinkTransparentColor = RGB(1, 2, 3);
static constexpr DWORD kLinkTransparentDibPixel = 0x00010203;

static bool RenderLinkWindow(HWND window, const wchar_t* text,
                             int x, int y, int width, int height);
struct MainWindowCandidate { HWND window = nullptr; LONG64 area = 0; };
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
    if (translation == g_translations.end()) return text;

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

static int FilterMemoryReadException(DWORD exceptionCode) {
    return exceptionCode == EXCEPTION_ACCESS_VIOLATION ||
           exceptionCode == EXCEPTION_IN_PAGE_ERROR
        ? EXCEPTION_EXECUTE_HANDLER
        : EXCEPTION_CONTINUE_SEARCH;
}

static const char* TranslateSafely(const char* text) {
    __try {
        return TranslateText(text);
    // Toolbag owns the incoming pointer. A stale engine pointer must fall back
    // to the original value instead of taking down the host process.
    } __except (FilterMemoryReadException(GetExceptionCode())) {
        return text;
    }
}

// ---------------------------------------------------------------------------
// Engine hooks (replacement functions)
// ---------------------------------------------------------------------------
// Font::CompiledString - translate before the compiled result is built/measured.
static void InvokeTranslatedFontCompile(int hookIndex, void* instance,
                                        const char* text) {
    const char* translated = TranslateSafely(text);
    g_originalFontCompile[hookIndex](instance, translated);
}
static void HookPrimaryFontCompile(void* instance, const char* text) {
    InvokeTranslatedFontCompile(0, instance, text);
}
static void HookSecondaryFontCompile(void* instance, const char* text) {
    InvokeTranslatedFontCompile(1, instance, text);
}

// Measure exactly the same translated string that the compile/draw path sees.
// No widget-owned text is changed, so command names, enum values and callbacks
// remain English internally while layout receives the Chinese glyph bounds.
static void HookFontMeasure(void* instance, float* width, float* height,
                            const char* text, int characterLimit,
                            bool singleLine) {
    const char* translated = TranslateSafely(text);
    g_originalFontMeasure(instance, width, height, translated,
                          characterLimit, singleLine);
}

// Capture the real child controls of TitleBar's window-button container after
// Toolbag has performed its own layout. Control stores its child vector at
// +0xF0/+0xF8; TitleBar stores the button container at +0x140.
static void HookTitleBarLayout(void* instance, void* layoutContext) {
    g_originalTitleBarLayout(instance, layoutContext);

    TitleBarSnapshot snapshot = {};
    __try {
        if (!instance) __leave;
        BYTE* titleBar = static_cast<BYTE*>(instance);
        BYTE* container = *reinterpret_cast<BYTE**>(titleBar + 0x140);
        if (!container || *reinterpret_cast<BYTE**>(container) != g_windowVtable)
            __leave;
        BYTE** begin = *reinterpret_cast<BYTE***>(container + 0xF0);
        BYTE** end = *reinterpret_cast<BYTE***>(container + 0xF8);
        if (!begin || !end || end < begin || end - begin != 3) __leave;
        for (BYTE** child = begin; child != end; ++child) {
            if (!*child || *reinterpret_cast<BYTE**>(*child) != g_buttonVtable)
                __leave;
        }
        constexpr size_t layoutOffsets[8] = {
            0x2C, 0x30, 0x4C, 0x50, 0x6C, 0x70, 0x8C, 0x90
        };
        for (size_t field = 0; field < 8; ++field) {
            snapshot.titleBarLayout[field] = *reinterpret_cast<float*>(
                titleBar + layoutOffsets[field]);
            snapshot.containerLayout[field] = *reinterpret_cast<float*>(
                container + layoutOffsets[field]);
            snapshot.minimizeButtonLayout[field] = *reinterpret_cast<float*>(
                begin[0] + layoutOffsets[field]);
        }
        snapshot.valid = true;
    } __except (FilterMemoryReadException(GetExceptionCode())) {
        snapshot.valid = false;
    }

    AcquireSRWLockExclusive(&g_titleBarSnapshotLock);
    g_titleBarSnapshot = snapshot;
    ReleaseSRWLockExclusive(&g_titleBarSnapshotLock);
}

static bool ReadTitleBarSnapshot(TitleBarSnapshot* output) {
    if (!output) return false;
    AcquireSRWLockShared(&g_titleBarSnapshotLock);
    *output = g_titleBarSnapshot;
    ReleaseSRWLockShared(&g_titleBarSnapshotLock);
    return output->valid;
}

static bool CalculateCaptionAnchorFromInternalLayout(
        const RECT& client, POINT origin, int* controlsLeft,
        int* controlsCenterY, int* renderedTitleBarHeight) {
    if (!controlsLeft || !controlsCenterY || !renderedTitleBarHeight ||
        client.right <= 0)
        return false;
    TitleBarSnapshot snapshot = {};
    if (!ReadTitleBarSnapshot(&snapshot)) return false;

    auto bounds = [](const float layout[8], float* left, float* top,
                     float* right, float* bottom) {
        *left = *right = layout[0];
        *top = *bottom = layout[1];
        for (size_t point = 1; point < 4; ++point) {
            const float x = layout[point * 2];
            const float y = layout[point * 2 + 1];
            if (!std::isfinite(x) || !std::isfinite(y)) return false;
            *left = x < *left ? x : *left;
            *right = x > *right ? x : *right;
            *top = y < *top ? y : *top;
            *bottom = y > *bottom ? y : *bottom;
        }
        return std::isfinite(*left) && std::isfinite(*top) &&
               *right > *left && *bottom > *top;
    };

    float titleLeft, titleTop, titleRight, titleBottom;
    float containerLeft, containerTop, containerRight, containerBottom;
    float buttonLeft, buttonTop, buttonRight, buttonBottom;
    if (!bounds(snapshot.titleBarLayout, &titleLeft, &titleTop,
                &titleRight, &titleBottom) ||
        !bounds(snapshot.containerLayout, &containerLeft, &containerTop,
                &containerRight, &containerBottom) ||
        !bounds(snapshot.minimizeButtonLayout, &buttonLeft, &buttonTop,
                &buttonRight, &buttonBottom)) return false;

    const float titleWidth = titleRight - titleLeft;
    const float titleHeight = titleBottom - titleTop;
    if (titleWidth <= 0.0f || titleHeight <= 0.0f) return false;
    const float scale = static_cast<float>(client.right) / titleWidth;
    if (!std::isfinite(scale) || scale <= 0.0f) return false;
    const float scaledHeight = titleHeight * scale;
    if (!std::isfinite(scaledHeight) || scaledHeight < 1.0f ||
        scaledHeight > static_cast<float>(INT_MAX)) return false;
    const int height = static_cast<int>(std::lround(scaledHeight));
    // Container and button coordinates are local to their respective parents.
    // TitleBar's own Y coordinates describe its placement in the root UI and
    // must not be subtracted from its children's local coordinates.
    const float internalLeft = containerLeft + buttonLeft;
    const float internalCenterY = containerTop +
        (buttonTop + buttonBottom) * 0.5f;
    *controlsLeft = origin.x + static_cast<int>(std::lround(internalLeft * scale));
    *controlsCenterY = origin.y +
        static_cast<int>(std::lround(internalCenterY * scale));
    *renderedTitleBarHeight = height;
    return true;
}

static void PositionLinkWindows() {
    if (!g_authorWindow || !g_gitHubWindow) return;
    if (!IsWindow(g_toolbagWindow)) {
        MainWindowCandidate candidate;
        EnumWindows(FindMainToolbagWindow,
                    reinterpret_cast<LPARAM>(&candidate));
        g_toolbagWindow = candidate.window;
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
    int controlsLeft = 0;
    int controlsCenterY = 0;
    int height = 0;
    if (!CalculateCaptionAnchorFromInternalLayout(
            client, origin, &controlsLeft, &controlsCenterY, &height)) {
        ShowWindow(g_authorWindow, SW_HIDE);
        ShowWindow(g_gitHubWindow, SW_HIDE);
        return;
    }
    const int margin = MulDiv(8, height, 45);
    const int gap = MulDiv(14, height, 45);
    if (height != g_authorHeight || g_authorWidth == 0 || g_gitHubWidth == 0) {
        HDC dc = GetDC(nullptr);
        if (dc) {
            HFONT font = CreateFontW(-MulDiv(20, height, 45), 0, 0, 0,
                                     FW_NORMAL, FALSE, TRUE, FALSE, DEFAULT_CHARSET,
                                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     ANTIALIASED_QUALITY, DEFAULT_PITCH,
                                     L"Microsoft YaHei UI");
            if (font) {
                HGDIOBJ oldFont = SelectObject(dc, font);
                SIZE size = {};
                if (oldFont && oldFont != HGDI_ERROR) {
                    if (GetTextExtentPoint32W(
                            dc, kAuthorText, static_cast<int>(wcslen(kAuthorText)),
                            &size))
                        g_authorWidth = size.cx;
                    if (GetTextExtentPoint32W(
                            dc, kGitHubText, static_cast<int>(wcslen(kGitHubText)),
                            &size))
                        g_gitHubWidth = size.cx;
                    SelectObject(dc, oldFont);
                }
                DeleteObject(font);
            }
            ReleaseDC(nullptr, dc);
        }
    }
    if (g_authorWidth <= 0) g_authorWidth = MulDiv(210, height, 45);
    if (g_gitHubWidth <= 0) g_gitHubWidth = MulDiv(100, height, 45);
    const int githubX = controlsLeft - g_gitHubWidth - margin;
    const int x = githubX - gap - g_authorWidth;
    const int y = controlsCenterY - height / 2;
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
    if (!referenceDc || !font || !text || width <= 0 || height <= 0) return 0;

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
    if (!oldBitmap || oldBitmap == HGDI_ERROR) {
        DeleteDC(memoryDc);
        DeleteObject(bitmap);
        return 0;
    }
    HFONT oldFont = (HFONT)SelectObject(memoryDc, font);
    if (!oldFont || oldFont == HGDI_ERROR) {
        SelectObject(memoryDc, oldBitmap);
        DeleteDC(memoryDc);
        DeleteObject(bitmap);
        return 0;
    }
    RECT rect = {0, 0, width, height};
    SetDCBrushColor(memoryDc, kLinkTransparentColor);
    FillRect(memoryDc, &rect, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    SetBkMode(memoryDc, TRANSPARENT);
    SetTextColor(memoryDc, kLinkTextColor);
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
    if (!window || !text || width <= 0 || height <= 0) return false;
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

    HFONT font = CreateFontW(-MulDiv(20, height, 45), 0, 0, 0,
                             FW_NORMAL, FALSE, TRUE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY, DEFAULT_PITCH,
                             L"Microsoft YaHei UI");
    if (!font) {
        DeleteObject(bitmap);
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        return false;
    }
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memoryDc, bitmap);
    if (!oldBitmap || oldBitmap == HGDI_ERROR) {
        DeleteObject(font);
        DeleteObject(bitmap);
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        return false;
    }
    HFONT oldFont = (HFONT)SelectObject(memoryDc, font);
    if (!oldFont || oldFont == HGDI_ERROR) {
        SelectObject(memoryDc, oldBitmap);
        DeleteObject(font);
        DeleteObject(bitmap);
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        return false;
    }
    memset(pixels, 0, static_cast<size_t>(width) * height * sizeof(DWORD));
    SetBkMode(memoryDc, TRANSPARENT);
    SetTextColor(memoryDc, kLinkTextColor);
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
    ShowWindow(window, updated ? SW_SHOWNOACTIVATE : SW_HIDE);
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
    RECT bounds{};
    if (!GetWindowRect(window, &bounds)) return TRUE;
    const LONG width = bounds.right - bounds.left;
    const LONG height = bounds.bottom - bounds.top;
    const LONG64 area = static_cast<LONG64>(width) * height;
    auto* candidate = reinterpret_cast<MainWindowCandidate*>(resultValue);
    if (area > candidate->area) {
        candidate->window = window;
        candidate->area = area;
    }
    return TRUE;
}

static DWORD WINAPI RunLinkWindowThread(LPVOID) {
    if (!g_toolbagMainThreadId) return 1;
    for (int i = 0; i < 300 && !g_toolbagWindow; i++) {
        MainWindowCandidate candidate;
        EnumThreadWindows(g_toolbagMainThreadId, FindMainToolbagWindow,
                          reinterpret_cast<LPARAM>(&candidate));
        g_toolbagWindow = candidate.window;
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

static bool ResolveCodeRange(BYTE* imageBase, BYTE*& codeBegin, BYTE*& codeEnd) {
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(imageBase);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(imageBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) return false;
    auto* section = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
        if (!(section->Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        codeBegin = imageBase + section->VirtualAddress;
        codeEnd = codeBegin + (std::max)(section->Misc.VirtualSize,
                                         section->SizeOfRawData);
        return true;
    }
    return false;
}

// Resolve a class' primary vftable from its RTTI mangled type name.
static BYTE* ResolveVtableByName(BYTE* imageBase, const char* rttiName) {
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(imageBase);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(imageBase + dos->e_lfanew);
    const size_t imageSize = nt->OptionalHeader.SizeOfImage;
    BYTE* codeBegin = nullptr;
    BYTE* codeEnd = nullptr;
    if (!ResolveCodeRange(imageBase, codeBegin, codeEnd)) return nullptr;
    const DWORD nameRva = FindBytesInImage(
        imageBase, rttiName, (DWORD)strlen(rttiName) + 1);
    if (nameRva < 0x10 || nameRva >= imageSize) return nullptr;
    const DWORD typeDescriptorRva = nameRva - 0x10;
    DWORD searchRva = 0;
    while (true) {
        const DWORD referenceRva = FindU32InImage(
            imageBase, typeDescriptorRva, searchRva);
        if (!referenceRva) break;
        searchRva = referenceRva + 1;
        if (referenceRva < 12 || referenceRva + sizeof(DWORD) > imageSize)
            continue;
        const DWORD locatorRva = referenceRva - 12;
        if (locatorRva + 24 > imageSize) continue;
        BYTE* locator = imageBase + locatorRva;
        const DWORD signature = *(DWORD*)(locator + 0x00);
        const DWORD referencedTypeDescriptor = *(DWORD*)(locator + 0x0C);
        if (signature > 1 || referencedTypeDescriptor != typeDescriptorRva)
            continue;
        const uintptr_t locatorAddress = (uintptr_t)imageBase + locatorRva;
        const DWORD vtableReferenceRva = FindU64InImage(imageBase, locatorAddress);
        if (vtableReferenceRva && vtableReferenceRva + 16 <= imageSize) {
            BYTE* vtable = imageBase + vtableReferenceRva + 8;
            BYTE* firstMethod = reinterpret_cast<BYTE*>(
                *reinterpret_cast<void**>(vtable));
            if (firstMethod >= codeBegin && firstMethod < codeEnd)
                return vtable;
        }
    }
    return nullptr;
}

constexpr size_t kAbsoluteJumpSize = 14;

struct DecodedInstruction {
    ZydisDecodedInstruction instruction{};
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
};

static const ZydisDecoder& X64Decoder() {
    static ZydisDecoder decoder = [] {
        ZydisDecoder value{};
        ZydisDecoderInit(&value, ZYDIS_MACHINE_MODE_LONG_64,
                         ZYDIS_STACK_WIDTH_64);
        return value;
    }();
    return decoder;
}

static bool DecodeInstruction(const BYTE* address, size_t available,
                              DecodedInstruction& decoded) {
    return ZYAN_SUCCESS(ZydisDecoderDecodeFull(
        &X64Decoder(), address,
        (std::min)(available, static_cast<size_t>(ZYDIS_MAX_INSTRUCTION_LENGTH)),
        &decoded.instruction, decoded.operands));
}

static bool IsRegisterOperand(const ZydisDecodedOperand& operand,
                              ZydisRegister value) {
    return operand.type == ZYDIS_OPERAND_TYPE_REGISTER &&
           operand.reg.value == value;
}

static BYTE* RelativeTarget(const BYTE* address,
                            const DecodedInstruction& decoded) {
    for (ZyanU8 i = 0; i < decoded.instruction.operand_count_visible; ++i) {
        const auto& operand = decoded.operands[i];
        if (operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
            operand.imm.is_relative) {
            return const_cast<BYTE*>(address) + decoded.instruction.length +
                   operand.imm.value.s;
        }
    }
    return nullptr;
}

static bool IsDirectCall(const BYTE* address,
                         const DecodedInstruction& decoded,
                         BYTE*& target) {
    if (decoded.instruction.mnemonic != ZYDIS_MNEMONIC_CALL) return false;
    target = RelativeTarget(address, decoded);
    return target != nullptr;
}

static bool IsExecutableAddress(BYTE* address, BYTE* codeBegin, BYTE* codeEnd) {
    return address >= codeBegin && address < codeEnd;
}

// Walk reachable basic blocks without following calls. Limits are safety
// budgets, not byte-layout assumptions; RET ends only the current branch.
static bool CollectReachableCalls(BYTE* entry, BYTE* codeBegin, BYTE* codeEnd,
                                  std::vector<BYTE*>& calls) {
    constexpr size_t kMaximumBlocks = 128;
    constexpr size_t kMaximumInstructions = 4096;
    std::vector<BYTE*> pending{entry};
    std::unordered_set<BYTE*> visited;
    size_t blocks = 0, instructions = 0;
    while (!pending.empty() && blocks++ < kMaximumBlocks &&
           instructions < kMaximumInstructions) {
        BYTE* p = pending.back();
        pending.pop_back();
        if (!IsExecutableAddress(p, codeBegin, codeEnd)) continue;
        while (IsExecutableAddress(p, codeBegin, codeEnd) &&
               instructions++ < kMaximumInstructions && visited.insert(p).second) {
            DecodedInstruction decoded;
            if (!DecodeInstruction(p, static_cast<size_t>(codeEnd - p), decoded))
                return false;
            BYTE* target = nullptr;
            if (IsDirectCall(p, decoded, target)) calls.push_back(target);
            const auto category = decoded.instruction.meta.category;
            if (category == ZYDIS_CATEGORY_COND_BR) {
                target = RelativeTarget(p, decoded);
                if (IsExecutableAddress(target, codeBegin, codeEnd))
                    pending.push_back(target);
            }
            if (category == ZYDIS_CATEGORY_UNCOND_BR) {
                target = RelativeTarget(p, decoded);
                if (IsExecutableAddress(target, codeBegin, codeEnd))
                    pending.push_back(target);
                break;
            }
            if (category == ZYDIS_CATEGORY_RET ||
                decoded.instruction.mnemonic == ZYDIS_MNEMONIC_INT3) break;
            p += decoded.instruction.length;
        }
    }
    std::sort(calls.begin(), calls.end());
    calls.erase(std::unique(calls.begin(), calls.end()), calls.end());
    return blocks <= kMaximumBlocks && instructions <= kMaximumInstructions;
}

static bool IsNonvolatileRegister(ZydisRegister value) {
    switch (value) {
        case ZYDIS_REGISTER_RBX:
        case ZYDIS_REGISTER_RBP:
        case ZYDIS_REGISTER_RSI:
        case ZYDIS_REGISTER_RDI:
        case ZYDIS_REGISTER_R12:
        case ZYDIS_REGISTER_R13:
        case ZYDIS_REGISTER_R14:
        case ZYDIS_REGISTER_R15:
            return true;
        default:
            return false;
    }
}

static bool LooksLikeMeasureFunction(BYTE* target, BYTE* imageEnd) {
    size_t offset = 0;
    bool preservesText = false;
    bool hasLargeWorkspace = false;
    bool callsStackProbe = false;
    ZydisRegister savedText = ZYDIS_REGISTER_NONE;

    while (offset < 0x90 && target + offset < imageEnd) {
        DecodedInstruction decoded;
        if (!DecodeInstruction(target + offset,
                               static_cast<size_t>(imageEnd - target - offset),
                               decoded)) return false;

        const auto mnemonic = decoded.instruction.mnemonic;
        if (mnemonic == ZYDIS_MNEMONIC_MOV &&
            decoded.instruction.operand_count_visible >= 2 &&
            decoded.operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
            decoded.operands[1].reg.value == ZYDIS_REGISTER_R9) {
            if (decoded.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                IsNonvolatileRegister(decoded.operands[0].reg.value)) {
                savedText = decoded.operands[0].reg.value;
                preservesText = true;
            } else if (decoded.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                       (decoded.operands[0].mem.base == ZYDIS_REGISTER_RSP ||
                        decoded.operands[0].mem.base == ZYDIS_REGISTER_RBP)) {
                preservesText = true;
            }
        }

        if (mnemonic == ZYDIS_MNEMONIC_MOV &&
            decoded.instruction.operand_count_visible >= 2 &&
            IsRegisterOperand(decoded.operands[0], ZYDIS_REGISTER_EAX) &&
            decoded.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
            decoded.operands[1].imm.value.u >= 0x400) {
            hasLargeWorkspace = true;
        }
        if (mnemonic == ZYDIS_MNEMONIC_SUB &&
            decoded.instruction.operand_count_visible >= 2 &&
            IsRegisterOperand(decoded.operands[0], ZYDIS_REGISTER_RSP)) {
            const auto& amount = decoded.operands[1];
            if ((amount.type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                 amount.imm.value.u >= 0x400) ||
                IsRegisterOperand(amount, ZYDIS_REGISTER_RAX)) {
                hasLargeWorkspace = true;
            }
        }
        if (mnemonic == ZYDIS_MNEMONIC_CALL && hasLargeWorkspace)
            callsStackProbe = true;

        // A null test may use R9 directly or the nonvolatile register chosen
        // by this particular compiler build. It strengthens the match but is
        // not required because older builds perform it later in the function.
        if ((mnemonic == ZYDIS_MNEMONIC_TEST || mnemonic == ZYDIS_MNEMONIC_CMP) &&
            decoded.instruction.operand_count_visible >= 1 &&
            decoded.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
            (decoded.operands[0].reg.value == ZYDIS_REGISTER_R9 ||
             decoded.operands[0].reg.value == savedText)) {
            preservesText = true;
        }

        offset += decoded.instruction.length;
    }
    return preservesText && hasLargeWorkspace && callsStackProbe;
}

static bool LooksLikeFontCompileFunction(BYTE* target, BYTE* imageEnd) {
    if (target < reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr)) ||
        target >= imageEnd) return false;
    bool hasNullGuard = false;
    bool usesText = false;
    bool previousWasNullCheck = false;
    size_t offset = 0;
    while (offset < 0x60 && target + offset < imageEnd) {
        DecodedInstruction decoded;
        if (!DecodeInstruction(target + offset,
                               static_cast<size_t>(imageEnd - target - offset),
                               decoded)) return false;
        const auto mnemonic = decoded.instruction.mnemonic;
        if (previousWasNullCheck && mnemonic == ZYDIS_MNEMONIC_JZ &&
            RelativeTarget(target + offset, decoded)) {
            hasNullGuard = true;
        }
        previousWasNullCheck = false;
        if (decoded.instruction.operand_count_visible >= 1 &&
            (mnemonic == ZYDIS_MNEMONIC_TEST || mnemonic == ZYDIS_MNEMONIC_CMP) &&
            IsRegisterOperand(decoded.operands[0], ZYDIS_REGISTER_RDX)) {
            if ((mnemonic == ZYDIS_MNEMONIC_TEST &&
                 decoded.instruction.operand_count_visible >= 2 &&
                 IsRegisterOperand(decoded.operands[1], ZYDIS_REGISTER_RDX)) ||
                (mnemonic == ZYDIS_MNEMONIC_CMP &&
                 decoded.instruction.operand_count_visible >= 2 &&
                 decoded.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                 decoded.operands[1].imm.value.u == 0)) {
                previousWasNullCheck = true;
            }
        }
        for (ZyanU8 i = 0; i < decoded.instruction.operand_count_visible; ++i) {
            const auto& operand = decoded.operands[i];
            if ((operand.type == ZYDIS_OPERAND_TYPE_MEMORY &&
                 operand.mem.base == ZYDIS_REGISTER_RDX) ||
                (i > 0 && operand.type == ZYDIS_OPERAND_TYPE_REGISTER &&
                 operand.reg.value == ZYDIS_REGISTER_RDX &&
                 decoded.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                 IsNonvolatileRegister(decoded.operands[0].reg.value))) {
                usesText = true;
            }
        }
        offset += decoded.instruction.length;
        if (mnemonic == ZYDIS_MNEMONIC_RET) break;
    }
    return hasNullGuard && usesText;
}

struct TextMethodScore {
    int compileTargets = 0;
    int measureTargets = 0;
};

static TextMethodScore ScoreTextMethod(BYTE* method, BYTE* imageBegin,
                                       BYTE* imageEnd) {
    TextMethodScore score;
    std::vector<BYTE*> calls;
    if (!CollectReachableCalls(method, imageBegin, imageEnd, calls)) return score;
    for (BYTE* target : calls) {
        if (!IsExecutableAddress(target, imageBegin, imageEnd)) continue;
        if (LooksLikeFontCompileFunction(target, imageEnd))
            ++score.compileTargets;
        if (LooksLikeMeasureFunction(target, imageEnd))
            ++score.measureTargets;
    }
    return score;
}

static BYTE* ResolveTextDrawMethod(BYTE* textVtable, BYTE* imageBegin,
                                   BYTE* imageEnd) {
    BYTE* bestMethod = nullptr;
    int bestScore = 0;
    bool ambiguous = false;
    constexpr size_t kMaximumVtableSlots = 64;
    for (size_t slot = 0; slot < kMaximumVtableSlots; ++slot) {
        BYTE* method = reinterpret_cast<BYTE*>(
            *reinterpret_cast<void**>(textVtable + slot * sizeof(void*)));
        if (method < imageBegin || method >= imageEnd) break;
        const TextMethodScore score = ScoreTextMethod(method, imageBegin,
                                                      imageEnd);
        if (!score.compileTargets || score.measureTargets != 1) continue;
        const int value = score.compileTargets * 10 + score.measureTargets;
        if (value > bestScore) {
            bestMethod = method;
            bestScore = value;
            ambiguous = false;
        } else if (value == bestScore && method != bestMethod) {
            ambiguous = true;
        }
    }
    return ambiguous ? nullptr : bestMethod;
}

// Identify TitleBar's layout override by the set of class fields it consumes.
// Unlike a fixed vtable slot, these semantic accesses have remained stable as
// methods move between slots/builds: menu bar (+0x138), caption container
// (+0x140), frame (+0x148), and the two title textures (+0x150/+0x158).
static bool LooksLikeTitleBarLayoutMethod(BYTE* method, BYTE* codeEnd) {
    constexpr ZyanI64 requiredOffsets[] = {
        0x138, 0x140, 0x148, 0x150, 0x158
    };
    bool found[ARRAYSIZE(requiredOffsets)] = {};
    size_t offset = 0;
    size_t instructions = 0;
    while (offset < 0x500 && method + offset < codeEnd &&
           instructions++ < 512) {
        DecodedInstruction decoded;
        if (!DecodeInstruction(method + offset,
                               static_cast<size_t>(codeEnd - method - offset),
                               decoded)) return false;
        for (ZyanU8 operandIndex = 0;
             operandIndex < decoded.instruction.operand_count_visible;
             ++operandIndex) {
            const auto& operand = decoded.operands[operandIndex];
            if (operand.type != ZYDIS_OPERAND_TYPE_MEMORY) continue;
            for (size_t field = 0; field < ARRAYSIZE(requiredOffsets); ++field)
                if (operand.mem.disp.value == requiredOffsets[field])
                    found[field] = true;
        }
        offset += decoded.instruction.length;
        if (decoded.instruction.meta.category == ZYDIS_CATEGORY_RET) break;
    }
    for (bool value : found)
        if (!value) return false;
    return true;
}

static BYTE* ResolveTitleBarLayoutMethod(BYTE* titleBarVtable,
                                         BYTE* codeBegin, BYTE* codeEnd) {
    if (!titleBarVtable) return nullptr;
    BYTE* match = nullptr;
    constexpr size_t kMaximumVtableSlots = 64;
    for (size_t slot = 0; slot < kMaximumVtableSlots; ++slot) {
        BYTE* method = reinterpret_cast<BYTE*>(
            *reinterpret_cast<void**>(titleBarVtable + slot * sizeof(void*)));
        if (!IsExecutableAddress(method, codeBegin, codeEnd)) break;
        if (!LooksLikeTitleBarLayoutMethod(method, codeEnd)) continue;
        if (match && match != method) return nullptr;
        match = method;
    }
    return match;
}

static void EmitJump(BYTE* destination, const void* target);
static bool InstallInlineHook(BYTE* target, void* replacement,
                              void** original);

static bool InstallFontHook(BYTE* target, int hookIndex);
static bool InstallMeasureHook(BYTE* target);
// Scan a Text method for calls whose targets guard and consume the RDX text
// argument, then install the shared instruction-relocating inline hook.
static bool InstallFontHooksFromTextDraw(BYTE* textDrawMethod,
                                         BYTE* imageBegin, BYTE* imageEnd) {
    bool any = false;
    int installedCount = 0;
    std::vector<BYTE*> calls;
    if (!CollectReachableCalls(textDrawMethod, imageBegin, imageEnd, calls))
        return false;
    for (BYTE* target : calls) {
        if (installedCount >= 2) break;
        if (IsExecutableAddress(target, imageBegin, imageEnd) &&
            LooksLikeFontCompileFunction(target, imageEnd) &&
            InstallFontHook(target, installedCount)) {
            ++installedCount;
            any = true;
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
    BYTE* match = nullptr;
    std::vector<BYTE*> calls;
    if (!CollectReachableCalls(textDrawMethod, imageBegin, imageEnd, calls))
        return false;
    for (BYTE* target : calls) {
        if (IsExecutableAddress(target, imageBegin, imageEnd) &&
            LooksLikeMeasureFunction(target, imageEnd)) {
            if (match && match != target) return false; // ambiguous: never guess
            match = target;
        }
    }
    return match && InstallMeasureHook(match);
}

// Register-preserving absolute jump: jmp qword ptr [rip+0]; <address>.
static void EmitJump(BYTE* p, const void* target) {
    p[0] = 0xff;
    p[1] = 0x25;
    *reinterpret_cast<uint32_t*>(p + 2) = 0;
    *reinterpret_cast<const void**>(p + 6) = target;
}

static bool InstallFontHook(BYTE* target, int hookIndex) {
    if (hookIndex < 0 || hookIndex >= 2 || g_originalFontCompile[hookIndex])
        return false;
    void* original = nullptr;
    if (!InstallInlineHook(target,
                           hookIndex == 0 ? (void*)HookPrimaryFontCompile
                                          : (void*)HookSecondaryFontCompile,
                           &original)) return false;
    g_originalFontCompile[hookIndex] = reinterpret_cast<FontCompileFn>(original);
    return true;
}

static bool WriteSignedDisplacement(BYTE* destination, ZyanU8 bitCount,
                                    int64_t value) {
    switch (bitCount) {
        case 8:
            if (value < INT8_MIN || value > INT8_MAX) return false;
            *reinterpret_cast<int8_t*>(destination) = static_cast<int8_t>(value);
            return true;
        case 16:
            if (value < INT16_MIN || value > INT16_MAX) return false;
            *reinterpret_cast<int16_t*>(destination) = static_cast<int16_t>(value);
            return true;
        case 32:
            if (value < INT32_MIN || value > INT32_MAX) return false;
            *reinterpret_cast<int32_t*>(destination) = static_cast<int32_t>(value);
            return true;
        default:
            return false;
    }
}

static bool CopyRelocatedInstructions(BYTE* destination, BYTE* source,
                                      size_t length) {
    size_t offset = 0;
    while (offset < length) {
        DecodedInstruction decoded;
        if (!DecodeInstruction(source + offset, length - offset, decoded) ||
            offset + decoded.instruction.length > length)
            return false;
        memcpy(destination + offset, source + offset,
               decoded.instruction.length);

        bool ripRelative = false;
        for (ZyanU8 i = 0; i < decoded.instruction.operand_count_visible; ++i) {
            if (decoded.operands[i].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                decoded.operands[i].mem.base == ZYDIS_REGISTER_RIP) {
                ripRelative = true;
                break;
            }
        }
        if (ripRelative) {
            const auto& displacement = decoded.instruction.raw.disp;
            const BYTE* absolute = source + offset + decoded.instruction.length +
                                   displacement.value;
            const int64_t relocated = absolute -
                (destination + offset + decoded.instruction.length);
            if (!WriteSignedDisplacement(destination + offset + displacement.offset,
                                         displacement.size, relocated))
                return false;
        }

        for (const auto& immediate : decoded.instruction.raw.imm) {
            if (!immediate.is_relative || !immediate.size) continue;
            BYTE* absolute = source + offset + decoded.instruction.length +
                             immediate.value.s;
            if (absolute >= source && absolute < source + length)
                absolute = destination + (absolute - source);
            const int64_t relocated = absolute -
                (destination + offset + decoded.instruction.length);
            if (!WriteSignedDisplacement(destination + offset + immediate.offset,
                                         immediate.size, relocated))
                return false;
        }
        offset += decoded.instruction.length;
    }
    return offset == length;
}

static size_t CompleteInstructionSpan(BYTE* target, size_t minimum,
                                      size_t maximum) {
    size_t length = 0;
    while (length < minimum && length < maximum) {
        DecodedInstruction decoded;
        if (!DecodeInstruction(target + length, maximum - length, decoded))
            return 0;
        length += decoded.instruction.length;
    }
    return length >= minimum ? length : 0;
}

static BYTE* AllocateExecutableNear(BYTE* target, size_t size) {
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const uintptr_t granularity = systemInfo.dwAllocationGranularity;
    const uintptr_t targetAddress = reinterpret_cast<uintptr_t>(target);
    const uintptr_t minimumAddress = (std::max)(
        reinterpret_cast<uintptr_t>(systemInfo.lpMinimumApplicationAddress),
        targetAddress > static_cast<uintptr_t>(INT32_MAX)
            ? targetAddress - static_cast<uintptr_t>(INT32_MAX)
            : uintptr_t{0});
    const uintptr_t maximumApplicationAddress =
        reinterpret_cast<uintptr_t>(systemInfo.lpMaximumApplicationAddress);
    const uintptr_t maximumAddress = (std::min)(
        maximumApplicationAddress - size,
        targetAddress <= maximumApplicationAddress - static_cast<uintptr_t>(INT32_MAX)
            ? targetAddress + static_cast<uintptr_t>(INT32_MAX)
            : maximumApplicationAddress);

    struct Candidate { uintptr_t address; uintptr_t distance; };
    std::vector<Candidate> candidates;
    uintptr_t cursor = minimumAddress;
    while (cursor < maximumAddress) {
        MEMORY_BASIC_INFORMATION region{};
        if (!VirtualQuery(reinterpret_cast<void*>(cursor), &region,
                          sizeof(region))) break;
        const uintptr_t regionStart = reinterpret_cast<uintptr_t>(region.BaseAddress);
        const uintptr_t regionEnd = regionStart + region.RegionSize;
        if (region.State == MEM_FREE && region.RegionSize >= size) {
            const uintptr_t first = (regionStart + granularity - 1) &
                                    ~(granularity - 1);
            const uintptr_t last = (regionEnd - size) & ~(granularity - 1);
            if (first <= last && last >= minimumAddress && first <= maximumAddress) {
                uintptr_t preferred = targetAddress & ~(granularity - 1);
                preferred = (std::max)(preferred, first);
                preferred = (std::min)(preferred, last);
                const uintptr_t distance = preferred > targetAddress
                    ? preferred - targetAddress : targetAddress - preferred;
                candidates.push_back({preferred, distance});
            }
        }
        if (regionEnd <= cursor) break;
        cursor = regionEnd;
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& left, const Candidate& right) {
                  return left.distance < right.distance;
              });
    for (const Candidate& candidate : candidates) {
        if (void* allocation = VirtualAlloc(
                reinterpret_cast<void*>(candidate.address), size,
                MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)) {
            return static_cast<BYTE*>(allocation);
        }
    }
    return static_cast<BYTE*>(VirtualAlloc(
        nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
}

static bool InstallInlineHook(BYTE* target, void* replacement,
                              void** original) {
    if (!target || !replacement || !original) return false;
    const size_t patchSize = CompleteInstructionSpan(
        target, kAbsoluteJumpSize, 64);
    if (!patchSize) return false;
    BYTE* trampoline = AllocateExecutableNear(
        target, patchSize + kAbsoluteJumpSize);
    if (!trampoline) return false;
    if (!CopyRelocatedInstructions(trampoline, target, patchSize)) {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return false;
    }
    EmitJump(trampoline + patchSize, target + patchSize);

    DWORD oldProtect = 0;
    if (!VirtualProtect(target, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return false;
    }
    EmitJump(target, replacement);
    for (size_t i = kAbsoluteJumpSize; i < patchSize; ++i) target[i] = 0x90;
    DWORD ignored = 0;
    VirtualProtect(target, patchSize, oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), target, patchSize);
    *original = trampoline;
    return true;
}

static bool InstallMeasureHook(BYTE* target) {
    if (g_originalFontMeasure) return true;
    void* original = nullptr;
    if (!InstallInlineHook(target, reinterpret_cast<void*>(HookFontMeasure),
                           &original)) return false;
    g_originalFontMeasure = reinterpret_cast<FontMeasureFn>(original);
    return true;
}

// ---------------------------------------------------------------------------
// Installation
// ---------------------------------------------------------------------------
static BOOL CALLBACK InitializeHooks(PINIT_ONCE, PVOID, PVOID*) {
    wchar_t startupInfoName[96];
    swprintf_s(startupInfoName, L"Local\\ToolbagChineseStartup_%lu",
               GetCurrentProcessId());
    HANDLE startupMapping = OpenFileMappingW(FILE_MAP_READ, FALSE,
                                             startupInfoName);
    if (startupMapping) {
        const DWORD* sharedMainThreadId = static_cast<const DWORD*>(
            MapViewOfFile(startupMapping, FILE_MAP_READ, 0, 0, sizeof(DWORD)));
        if (sharedMainThreadId) {
            g_toolbagMainThreadId = *sharedMainThreadId;
            UnmapViewOfFile(sharedMainThreadId);
        }
        CloseHandle(startupMapping);
    }
    if (!LoadDictionary()) return TRUE;

    BYTE* imageBase = (BYTE*)GetModuleHandleW(nullptr);
    BYTE* codeBegin = nullptr;
    BYTE* codeEnd = nullptr;
    if (!ResolveCodeRange(imageBase, codeBegin, codeEnd)) return TRUE;

    // ----------------------------------------------------------------------
    // Self-adaptive hooking: each layer is located & installed independently.
    // If a newer Toolbag version changes one function's machine-code, only
    // that layer is skipped - the others still translate. (auto one-click)
    // ----------------------------------------------------------------------

    // Resolve the mset::Text vtable once (version-stable RTTI anchor).
    BYTE* textVtable = ResolveVtableByName(imageBase, ".?AVText@mset@@");

    // 1) Font::CompiledString - RTTI/vtable. The draw slot moved between
    //    Toolbag 5.0.0 and later builds, so select it by its font call graph.
    //    This is the only translation layer: it draws with Chinese while the
    //    engine's stored strings stay English, so command/callback keys remain
    //    intact (buttons work).
    if (textVtable) {
        BYTE* textDrawMethod = ResolveTextDrawMethod(
            textVtable, codeBegin, codeEnd);
        if (textDrawMethod) {
            const bool compileReady =
                InstallFontHooksFromTextDraw(textDrawMethod, codeBegin, codeEnd);
            const bool measureReady =
                InstallMeasureHookFromTextDraw(textDrawMethod, codeBegin, codeEnd);
            // Drawing remains the required base layer.  Measurement improves
            // layout when available, but a version mismatch must not prevent
            // the safe display-only translation from starting.
            if (compileReady) g_hooksInstalled = TRUE;
            (void)measureReady;
        }
    }

    // Resolve the caption layout by class RTTI plus method semantics. Never
    // assume a fixed RVA or vtable slot; a mismatch only disables link
    // positioning and cannot prevent the translation hooks from starting.
    BYTE* titleBarVtable = ResolveVtableByName(imageBase, ".?AVTitleBar@mset@@");
    g_windowVtable = ResolveVtableByName(imageBase, ".?AVWindow@mset@@");
    g_buttonVtable = ResolveVtableByName(imageBase, ".?AVButton@mset@@");
    if (titleBarVtable && g_windowVtable && g_buttonVtable) {
        BYTE* titleBarLayout = ResolveTitleBarLayoutMethod(
            titleBarVtable, codeBegin, codeEnd);
        void* original = nullptr;
        if (titleBarLayout &&
            InstallInlineHook(titleBarLayout,
                              reinterpret_cast<void*>(HookTitleBarLayout),
                              &original)) {
            g_originalTitleBarLayout =
                reinterpret_cast<TitleBarLayoutFn>(original);
        }
    }

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
