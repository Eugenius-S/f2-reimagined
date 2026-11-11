#pragma once

// What "Remove mod" and "Remove the Russian translation" change (task 234).
// Plain text and name lists only, no windows: the launcher does the file work
// and tests/launcher_progress/launcher_uninstall_test.cc checks this part.

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace launcher_uninstall {

// Files the installer put in the game folder, besides the launcher itself
// (that one goes last, it may be the running program).
inline const std::vector<std::string>& installedFiles()
{
    static const std::vector<std::string> names = {"fallout2-ce.exe", "ce.dat", "scene-exporter.exe", "version.txt"};
    return names;
}

// Folders only the mod makes: the 3D scene data and the old builds kept on update.
inline const std::vector<std::string>& installedFolders()
{
    static const std::vector<std::string> names = {"mod-cache", "f2reimagined-backups"};
    return names;
}

inline std::string lowered(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return text;
}

inline std::string trimmed(const std::string& text)
{
    size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return std::string();
    return text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
}

// The text without the lines that are exactly `name` (any case); other lines and their endings stay as they were.
inline std::string removeLine(const std::string& text, const std::string& name, bool* removed = nullptr)
{
    std::string out;
    bool any = false;
    size_t begin = 0;
    while (begin < text.size()) {
        size_t end = text.find('\n', begin);
        end = end == std::string::npos ? text.size() : end + 1;
        std::string line = text.substr(begin, end - begin);
        if (lowered(trimmed(line)) == lowered(name)) any = true;
        else out += line;
        begin = end;
    }
    if (removed != nullptr) *removed = any;
    return out;
}

// True when some line has text on it.
inline bool hasContent(const std::string& text)
{
    return !trimmed(text).empty();
}

// The text with [system] language= set to `language`; *found tells whether the key was there.
inline std::string withLanguage(const std::string& input, const std::string& language, bool* found = nullptr)
{
    std::string output;
    bool inSystem = false;
    bool any = false;
    size_t begin = 0;
    while (begin < input.size()) {
        size_t end = input.find('\n', begin);
        end = end == std::string::npos ? input.size() : end + 1;
        std::string line = input.substr(begin, end - begin);
        std::string content = line;
        while (!content.empty() && (content.back() == '\r' || content.back() == '\n')) content.pop_back();
        if (!content.empty() && content[0] == '[') inSystem = content == "[system]";
        if (inSystem && content.rfind("language=", 0) == 0) {
            line = "language=" + language + line.substr(content.size());
            any = true;
        }
        output += line;
        begin = end;
    }
    if (found != nullptr) *found = any;
    return output;
}

// [system] language= of a config text, empty when there is none.
inline std::string languageOf(const std::string& input)
{
    bool inSystem = false;
    size_t begin = 0;
    while (begin < input.size()) {
        size_t end = input.find('\n', begin);
        end = end == std::string::npos ? input.size() : end + 1;
        std::string content = trimmed(input.substr(begin, end - begin));
        if (!content.empty() && content[0] == '[') inSystem = content == "[system]";
        if (inSystem && content.rfind("language=", 0) == 0) return lowered(trimmed(content.substr(9)));
        begin = end;
    }
    return std::string();
}

} // namespace launcher_uninstall
