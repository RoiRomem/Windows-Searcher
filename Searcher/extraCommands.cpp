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

void RunExtraCommands(SHELLEXECUTEINFOW &sei, std::wstring cmd) {

    // Convert to lowercase for case-insensitive comparison
    cmd = strtowlwr(cmd);

    // Check if the clean buffer matches any command
    bool commandFound = false;
    for (const auto& [cmdKey, cmdValue] : commandMap) {
        if (strtowlwr(cmdKey) == cmd) {
            sei.lpVerb = L"open";
            sei.lpFile = cmdValue.first.c_str();
            sei.lpParameters = cmdValue.second.c_str();

            // Execute the command
            if (!ShellExecuteExW(&sei)) {
                std::cerr << "Failed to execute command from commandMap. Error: " << GetLastError() << std::endl;
            }
            commandFound = true;
            break;
        }
    }

    // For non-command inputs, use OpenGoogle
    if (!commandFound) {
        OpenGoogle(sei);
    }
}

void OpenGoogle(SHELLEXECUTEINFOW &sei) {
    const std::wstring GOOGLE_LINKER = L"https://www.google.com/search?q=";

    // URL encode the buffer for Google search
    std::wstring encodedSearch;
    for (size_t i = 0; i < wcslen(buffer); i++) {
        wchar_t c = buffer[i];
        if (iswalnum(c) || c == L'-' || c == L'_' || c == L'.' || c == L'~') {
            encodedSearch += c;
        } else if (c == L' ') {
            encodedSearch += L'+';
        } else {
            wchar_t hex[4];
            swprintf(hex, 4, L"%%%02X", static_cast<unsigned int>(c));
            encodedSearch += hex;
        }
    }

    std::wstring search = GOOGLE_LINKER + encodedSearch;

    // Use ShellExecuteW directly
    ShellExecuteW(nullptr, L"open", search.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}