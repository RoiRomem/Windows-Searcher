#include "Searcher.h"

using namespace std;

template <typename K, typename V>
bool itemExists(const std::unordered_map<K, V>& map, const K& key) {
    return map.contains(key); 
}


void SetCommands() {
    commandMap["shutdown"] = {L"shutdown", L"/s /t 0"};
    commandMap["restart"] = {L"restart", L"/s /t 0"};
    commandMap["exit"] = {L"", L""};
    commandMap["reload"] = {L"", L""};
}

void RunExtraCommands(SHELLEXECUTEINFOW &sei) {
    if (itemExists(commandMap, removeWhitespace(buffer))) {
        sei.lpVerb = L"open";
        sei.lpFile = commandMap[buffer].first.c_str();
        sei.lpParameters = commandMap[buffer].second.c_str();

        // Execute the command here
        if (!ShellExecuteExW(&sei)) {
            std::cerr << "Failed to execute command from commandMap." << std::endl;
        }
    } else {
        // For non-command inputs like "hello world", use OpenGoogle
        OpenGoogle(sei);
    }
}

void OpenGoogle(SHELLEXECUTEINFOW &sei) {
    const std::wstring GOOGLE_LINKER = L"https://www.google.com/search?q=";

    // Convert char buffer (UTF-8) to wide string
    wchar_t wbuffer[256];
    MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wbuffer, 256);

    // Concatenate URL and search query
    const std::wstring search = GOOGLE_LINKER + wbuffer;

    // Use ShellExecuteW directly, not ShellExecuteExW
    ShellExecuteW(nullptr, L"open", search.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
