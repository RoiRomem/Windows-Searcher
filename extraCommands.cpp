#include "Searcher.h"

using namespace std;

template <typename K, typename V>
bool itemExists(const std::unordered_map<K, V>& map, const K& key) {
    return map.find(key) != map.end(); // Fixed for compatibility with older C++ standards
}

void SetCommands() {
    commandMap[L"shutdown"] = {L"shutdown", L"/s /t 0"};
    commandMap[L"restart"] = {L"shutdown", L"/r /t 0"};
    commandMap[L"exit"] = {L"", L""};
    commandMap[L"reload"] = {L"", L""};

    // Add more custom commands here
    commandMap[L"settings"] = {L"ms-settings:", L""};
    commandMap[L"calculator"] = {L"calc", L""};
    commandMap[L"notepad"] = {L"notepad", L""};
}

void RunExtraCommands(SHELLEXECUTEINFOW &sei) {
    // Fix: Use proper comparison with cleaned up buffer
    std::wstring cleanBuffer = removeWhitespace(buffer);
    if (itemExists(commandMap, cleanBuffer)) {
        sei.lpVerb = L"open";
        sei.lpFile = commandMap[cleanBuffer].first.c_str();
        sei.lpParameters = commandMap[cleanBuffer].second.c_str();

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

    // No need for additional conversion since buffer is already wchar_t
    std::wstring search = GOOGLE_LINKER + buffer;

    // Use ShellExecuteW directly, not ShellExecuteExW
    ShellExecuteW(nullptr, L"open", search.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}