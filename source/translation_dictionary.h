#pragma once
// Strict, transactional UTF-8 dictionary parsing. Runtime lookups remain immutable.
#include <windows.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace ToolbagDictionary {
using WideMap = std::unordered_map<std::wstring, std::wstring>;
class JsonReader {
public:
    explicit JsonReader(const std::wstring& source) : source_(source) {}

    bool ReadTranslations(WideMap& output,
                          std::wstring& error) {
        SkipWhitespace();
        if (!Consume(L'{')) return Fail(L"词库根节点必须是对象", error);
        std::unordered_set<std::wstring> fields;
        SkipWhitespace();
        if (position_ < source_.size() && source_[position_] != L'}') {
            while (true) {
                SkipWhitespace();
                std::wstring key;
                if (!ReadString(key) || !fields.insert(key).second) return Fail(L"无法读取词库字段名", error);
                SkipWhitespace();
                if (!Consume(L':')) return Fail(L"词库字段缺少冒号", error);
                SkipWhitespace();
                if (key == L"translations") {
                    if (!ReadStringMap(output)) return Fail(L"translations 必须是字符串映射", error);
                } else if (!SkipValue()) {
                    return Fail(L"词库包含无法解析的字段", error);
                }
                SkipWhitespace();
                if (position_ < source_.size() && source_[position_] == L'}') break;
                if (!Consume(L',')) return Fail(L"词库字段之间缺少逗号", error);
            }
        }
        if (!Consume(L'}')) return Fail(L"词库对象未结束", error);
        SkipWhitespace();
        if (position_ != source_.size()) return Fail(L"词库根节点后包含多余数据", error);
        if (output.empty()) return Fail(L"词库没有有效翻译", error);
        return true;
    }

private:
    void SkipWhitespace() {
        while (position_ < source_.size() && (source_[position_] == L' ' || source_[position_] == L'\t' || source_[position_] == L'\r' || source_[position_] == L'\n')) ++position_;
    }
    bool Consume(wchar_t expected) {
        if (position_ >= source_.size() || source_[position_] != expected) return false;
        ++position_;
        return true;
    }
    bool ReadString(std::wstring& output) {
        if (!Consume(L'"')) return false;
        output.clear();
        while (position_ < source_.size()) {
            wchar_t ch = source_[position_++];
            if (ch == L'"') {
                for (size_t i = 0; i < output.size(); ++i) {
                    const unsigned c = output[i];
                    if (c >= 0xd800 && c <= 0xdbff) {
                        if (++i == output.size() || output[i] < 0xdc00 || output[i] > 0xdfff) return false;
                    } else if (c >= 0xdc00 && c <= 0xdfff) return false;
                }
                return true;
            }
            if (ch < 0x20) return false;
            if (ch != L'\\') { output += ch; continue; }
            if (position_ >= source_.size()) return false;
            ch = source_[position_++];
            switch (ch) {
            case L'"': output += L'"'; break;
            case L'\\': output += L'\\'; break;
            case L'/': output += L'/'; break;
            case L'b': output += L'\b'; break;
            case L'f': output += L'\f'; break;
            case L'n': output += L'\n'; break;
            case L'r': output += L'\r'; break;
            case L't': output += L'\t'; break;
            case L'u': {
                if (position_ + 4 > source_.size()) return false;
                unsigned value = 0;
                for (int i = 0; i < 4; ++i) {
                    const wchar_t digit = source_[position_++];
                    value <<= 4;
                    if (digit >= L'0' && digit <= L'9') value += digit - L'0';
                    else if (digit >= L'a' && digit <= L'f') value += digit - L'a' + 10;
                    else if (digit >= L'A' && digit <= L'F') value += digit - L'A' + 10;
                    else return false;
                }
                output += static_cast<wchar_t>(value);
                break;
            }
            default: return false;
            }
        }
        return false;
    }
    bool ReadStringMap(WideMap& output) {
        if (!Consume(L'{')) return false;
        std::unordered_set<std::wstring> keys;
        SkipWhitespace();
        if (Consume(L'}')) return true;
        while (true) {
            SkipWhitespace();
            std::wstring source, translated;
            if (!ReadString(source) || !keys.insert(source).second) return false;
            SkipWhitespace();
            if (!Consume(L':')) return false;
            SkipWhitespace();
            if (!ReadString(translated) || source.find(L'\0') != std::wstring::npos ||
                translated.find(L'\0') != std::wstring::npos) return false;
            if (!source.empty() && !translated.empty()) output.emplace(std::move(source), std::move(translated));
            SkipWhitespace();
            if (Consume(L'}')) return true;
            if (!Consume(L',')) return false;
        }
    }
    bool SkipValue(unsigned depth = 0) {
        SkipWhitespace();
        if (depth > 64 || position_ >= source_.size()) return false;
        if (source_[position_] == L'"') { std::wstring ignored; return ReadString(ignored); }
        if (source_[position_] == L'{' || source_[position_] == L'[') {
            const bool object = source_[position_++] == L'{';
            const wchar_t close = object ? L'}' : L']';
            std::unordered_set<std::wstring> keys;
            SkipWhitespace();
            if (Consume(close)) return true;
            while (true) {
                SkipWhitespace();
                if (object) {
                    std::wstring key;
                    if (!ReadString(key) || !keys.insert(key).second) return false;
                    SkipWhitespace();
                    if (!Consume(L':')) return false;
                }
                if (!SkipValue(depth + 1)) return false;
                SkipWhitespace();
                if (Consume(close)) return true;
                if (!Consume(L',')) return false;
            }
        }
        for (const auto literal : {L"true", L"false", L"null"}) {
            const size_t length = wcslen(literal);
            if (source_.compare(position_, length, literal) == 0) { position_ += length; return true; }
        }
        Consume(L'-');
        auto digit = [&] { return position_ < source_.size() && source_[position_] >= L'0' && source_[position_] <= L'9'; };
        if (!digit()) return false;
        if (!Consume(L'0')) while (digit()) ++position_;
        if (Consume(L'.')) { if (!digit()) return false; while (digit()) ++position_; }
        if (Consume(L'e') || Consume(L'E')) {
            if (!Consume(L'+')) Consume(L'-');
            if (!digit()) return false;
            while (digit()) ++position_;
        }
        return true;
    }
    bool Fail(const wchar_t* message, std::wstring& error) {
        error = std::wstring(message) + L"，位置 " + std::to_wstring(position_);
        return false;
    }

    const std::wstring& source_;
    size_t position_{};
};


template<class Map>
bool Parse(const std::string& input, Map& output) {
    if (input.empty() || input.size() > 64 * 1024 * 1024) return false;
    const size_t bom = input.compare(0, 3, "\xef\xbb\xbf") == 0 ? 3 : 0;
    const char* bytes = input.data() + bom;
    const int size = static_cast<int>(input.size() - bom);
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes, size, nullptr, 0);
    if (count <= 0) return false;
    std::wstring text(static_cast<size_t>(count), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes, size, text.data(), count) != count) return false;
    WideMap parsed;
    std::wstring error;
    if (!JsonReader(text).ReadTranslations(parsed, error)) return false;
    Map next;
    next.reserve(parsed.size());
    auto utf8 = [](const std::wstring& value) {
        const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        std::string result(static_cast<size_t>(length), '\0');
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), length, nullptr, nullptr);
        return result;
    };
    for (const auto& item : parsed) next.emplace(utf8(item.first), utf8(item.second));
    output.swap(next);
    return true;
}
} // namespace ToolbagDictionary
