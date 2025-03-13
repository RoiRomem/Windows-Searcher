#include "Searcher.h"

using namespace std;

template <typename K, typename V>
bool itemExists(const std::unordered_map<K, V>& map, const K& key) {
    return map.contains(key); 
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

    // Convert buffer to wide string
    wchar_t wbuffer[256];
    // Use proper wide character handling for the buffer
    // Since buffer is already wchar_t in your code, just copy it directly
    wcscpy_s(wbuffer, 256, buffer);

    // Concatenate URL and search query
    const std::wstring search = GOOGLE_LINKER + wbuffer;

    // Use ShellExecuteW directly, not ShellExecuteExW
    ShellExecuteW(nullptr, L"open", search.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}