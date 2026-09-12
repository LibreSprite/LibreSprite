#pragma once

#include <cstdint>
#include <iostream>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace su {
    inline std::string breakLines(const std::string& str, size_t lineLength = 80) {
        std::string out;
        out.reserve(str.size());
        size_t currentLength = 0;
        for (char ch : str) {
            if (currentLength >= lineLength && ch <= ' ') ch = '\n';
            if (ch == '\n') currentLength = 0;
            else currentLength++;
            out.push_back(ch);
        }
        return out;
    }

    inline std::string btoa(std::string_view str) {
        static const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
        std::string result;
        result.reserve(((str.size() + 2) / 3) * 4);
        int val = 0, valb = -6;
        for (unsigned char c : str) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                result.push_back(base64_chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) {
            result.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
        }
        while (result.size() % 4) {
            result.push_back('=');
        }
        return result;
    }

    inline std::string atob(std::string_view str) {
        static const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
        std::string result;
        result.reserve((str.size() / 4) * 3);
        std::vector<int> T(256, -1);
        for (size_t i = 0; i < 64; i++) {
            T[static_cast<unsigned char>(base64_chars[i])] = i;
        }
        int val = 0, valb = -8;
        for (unsigned char c : str) {
            if (T[c] == -1) break;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                result.push_back(static_cast<char>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return result;
    }

    inline std::string escape(const std::string& str, bool dbl = true, bool sgl = false) {
        std::string escaped;
        escaped.reserve(str.size() + 2);
        escaped.push_back('"');
        for (char c : str) {
            switch (c) {
            case '\\': escaped.push_back('\\'); escaped.push_back('\\'); break;
            case '"':  if (dbl) escaped.push_back('\\'); escaped.push_back('"');  break;
            case '\'': if (sgl) escaped.push_back('\\'); escaped.push_back('\'');  break;
            case '\n': escaped.push_back('\\'); escaped.push_back('n');  break;
            case '\r': escaped.push_back('\\'); escaped.push_back('r');  break;
            case '\t': escaped.push_back('\\'); escaped.push_back('t');  break;
            case '\0': escaped.push_back('\\'); escaped.push_back('0');  break;
            default:   escaped.push_back(c); break;
            }
        }
        escaped.push_back('"');
        return escaped;
    }

    inline std::string unescape(const std::string& str) {
        std::string unescaped;
        unescaped.reserve(str.size());
        bool escapeNext = false;
        for (char c : str) {
            if (escapeNext) {
                switch (c) {
                case 'n': unescaped.push_back('\n'); break;
                case 'r': unescaped.push_back('\r'); break;
                case 't': unescaped.push_back('\t'); break;
                case '0': unescaped.push_back('\0'); break;
                case '\\': unescaped.push_back('\\'); break;
                case '"': unescaped.push_back('"'); break;
                default: unescaped.push_back(c); break;
                }
                escapeNext = false;
            } else {
                if (c == '\\') {
                    escapeNext = true;
                } else {
                    unescaped.push_back(c);
                }
            }
        }
        return unescaped;
    }

    inline std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == std::string::npos)
            return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, last - first + 1);
    }

    inline size_t levenshteinDistance(const std::string_view& s1, const std::string_view& s2) {
        size_t len1 = s1.size();
        size_t len2 = s2.size();
        std::vector<size_t> col(len2 + 1);
        for (size_t i = 0; i <= len2; i++)
            col[i] = i;
        for (size_t i = 1; i <= len1; i++) {
            col[0] = i;
            size_t lastdiag = i - 1;
            for (size_t j = 1; j <= len2; j++) {
                size_t olddiag = col[j];
                size_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
                col[j] = std::min({ col[j] + 1, col[j - 1] + 1, lastdiag + cost});
                lastdiag = olddiag;
            }
        }
        return col[len2];
    }

    inline float jaroWinklerDistance(const std::string_view& s1, const std::string_view& s2) {
        const float SCALING_FACTOR = 0.1f;
        size_t len1 = s1.size();
        size_t len2 = s2.size();
        if (len1 == 0) return len2 == 0 ? 1.0 : 0.0;
        size_t matchDistance = std::max(len1, len2) / 2 - 1;
        std::vector<bool> s1Matches(len1, false);
        std::vector<bool> s2Matches(len2, false);
        size_t matches = 0;
        for (size_t i = 0; i < len1; i++) {
            size_t start = (i >= matchDistance) ? i - matchDistance : 0;
            size_t end = std::min(i + matchDistance + 1, len2);
            for (size_t j = start; j < end; j++) {
                if (s2Matches[j]) continue;
                if (s1[i] != s2[j]) continue;
                s1Matches[i] = true;
                s2Matches[j] = true;
                matches++;
                break;
            }
        }
        if (matches == 0) return 0.0;
        size_t t = 0;
        size_t k = 0;
        for (size_t i = 0; i < len1; i++) {
            if (!s1Matches[i]) continue;
            while (!s2Matches[k]) k++;
            if (s1[i] != s2[k]) t++;
            k++;
        }
        t /= 2;
        float m = static_cast<float>(matches);
        float jaro = ((m / len1) + (m / len2) + ((m - t) / m)) / 3.0f;
        size_t prefix = 0;
        for (size_t i = 0; i < std::min({len1, len2, size_t(4)}); i++) {
            if (s1[i] == s2[i]) prefix++;
            else break;
        }
        return jaro + prefix * SCALING_FACTOR * (1.0f - jaro);
    }

    inline std::string replace(const std::string& str, const std::string_view& search, const std::string_view& replacement) {
        std::string result;
        size_t pos = 0;
        size_t searchLen = search.size();
        while (true) {
            size_t foundPos = str.find(search, pos);
            if (foundPos == std::string::npos) {
                result.append(str, pos, str.size() - pos);
                break;
            }
            result.append(str, pos, foundPos - pos);
            result.append(replacement);
            pos = foundPos + searchLen;
        }
        return result;
    }

    inline std::string replace(const std::string& str, const std::regex& expr, const std::string_view& replacement) {
        std::string result;
        std::sregex_iterator begin(str.begin(), str.end(), expr);
        std::sregex_iterator end;
        size_t lastPos = 0;
        for (auto it = begin; it != end; ++it) {
            result.append(str, lastPos, it->position() - lastPos);
            result.append(replacement);
            lastPos = it->position() + it->length();
        }
        result.append(str, lastPos, str.size() - lastPos);
        return result;
    }

    inline std::string replace(const std::string& str, const std::regex& expr, const std::string& replacement) {
        return replace(str, expr, std::string_view{replacement});
    }

    inline std::string replace(const std::string& str, const std::regex& expr, const char* replacement) {
        return replace(str, expr, std::string_view{replacement});
    }

    template<typename CB>
    std::string replace(const std::string& str, const std::regex& expr, CB&& cb) {
        std::string result;
        std::sregex_iterator begin(str.begin(), str.end(), expr);
        std::sregex_iterator end;
        size_t lastPos = 0;
        for (auto it = begin; it != end; ++it) {
            result.append(str, lastPos, it->position() - lastPos);
            result.append(cb(*it));
            lastPos = it->position() + it->length();
        }
        result.append(str, lastPos, str.size() - lastPos);
        return result;
    }

    template<typename CB>
    bool forEach(const std::string& str, const std::regex& expr, CB&& cb) {
        bool ret = false;
        std::sregex_iterator begin(str.begin(), str.end(), expr);
        std::sregex_iterator end;
        size_t lastPos = 0;
        for (auto it = begin; it != end; ++it) {
            ret = true;
            if constexpr (std::is_void_v<decltype(cb(*it))>) {
                cb(*it);
            } else {
                if (!cb(*it))
                    return false;
            }
            lastPos = it->position() + it->length();
        }
        return ret;
    }

    inline std::string toUpper(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            c = static_cast<char>(toupper(c));
        }
        return result;
    }

    inline std::string toLower(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            c = static_cast<char>(tolower(c));
        }
        return result;
    }

    inline std::vector<std::string> split(const std::string& str, const std::regex& delimiter) {
        std::vector<std::string> result;
        std::sregex_token_iterator begin(str.begin(), str.end(), delimiter, -1);
        std::sregex_token_iterator end;
        for (auto it = begin; it != end; ++it) {
            result.push_back(*it);
        }
        return result;
    }

    inline std::vector<std::pair<std::string, int>> tokenize(const std::string& str, const std::regex& expr) {
        std::vector<std::pair<std::string, int>> result;
        std::sregex_iterator begin(str.begin(), str.end(), expr);
        std::sregex_iterator end;
        for (auto it = begin; it != end; ++it) {
            for (size_t i = 1; i < it->size(); ++i) {
                if (!it->str(i).empty())
                    result.emplace_back(it->str(i), static_cast<int>(i));
            }
        }
        return result;
    }

    inline uint64_t hash(const std::string& str) {
        uint64_t hash = 14695981039346656037ULL;
        for (char c : str) {
            hash ^= static_cast<uint64_t>(c);
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    inline std::string join(const std::vector<std::string>& parts, const std::string& delimiter = "") {
        std::string result;
        for (size_t i = 0; i < parts.size(); ++i) {
            result += parts[i];
            if (i + 1 < parts.size()) {
                result += delimiter;
            }
        }
        return result;
    }

    inline std::string getDirname(const std::string& path) {
        size_t pos = path.find_last_of("/\\");
        if (pos == std::string::npos)
            return ".";
        return path.substr(0, pos);
    }

    inline std::string toUTF8(unsigned long codepoint) {
        std::string utf8;
        if (codepoint <= 0x7F) {
            utf8.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            utf8.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
            utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint <= 0xFFFF) {
            utf8.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
            utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint <= 0x10FFFF) {
            utf8.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
            utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        return utf8;
    }

    template<typename CB>
    void utf8ForEach(const std::string& utf8, CB&& cb) {
        unsigned long codepoint{};
        int remaining = 0;
        for (unsigned char c : utf8) {
            if (remaining == 0) {
                if (c <= 0x7F) {
                    cb(static_cast<unsigned long>(c));
                } else if ((c & 0xE0) == 0xC0) {
                    codepoint = c & 0x1F;
                    remaining = 1;
                } else if ((c & 0xF0) == 0xE0) {
                    codepoint = c & 0x0F;
                    remaining = 2;
                } else if ((c & 0xF8) == 0xF0) {
                    codepoint = c & 0x07;
                    remaining = 3;
                }
            } else {
                if ((c & 0xC0) == 0x80) {
                    codepoint = (codepoint << 6) | (c & 0x3F);
                    remaining--;
                    if (remaining == 0) {
                        cb(codepoint);
                    }
                } else {
                    remaining = 0;
                }
            }
        }
    }

    inline std::string generateUUID() {
        static std::mt19937_64 rng{std::random_device{}()};
        static std::uniform_int_distribution<uint64_t> dist{0, std::numeric_limits<uint64_t>::max()};
        uint64_t part1 = dist(rng);
        uint64_t part2 = dist(rng);
        std::stringstream ss;
        ss << std::hex;
        ss.width(16);
        ss.fill('0');
        ss << part1;
        ss.width(16);
        ss << part2;
        return ss.str();
    }
}
