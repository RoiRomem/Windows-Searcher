#include "Searcher.h"

std::wstring strtowlwr(std::wstring txt) {
    std::wstring result = txt;
    for (wchar_t& c : result) {
        c = std::towlower(c);
    }
    return result;
}

// Improved stringManipulator with more robust handling
std::wstring stringManipulator(const std::wstring& str, char delimiter) {
    if (str.empty()) {
        return L"";
    }

    // Get the filename part from the path
    size_t lastPos = str.find_last_of(L"\\/"); // Handle both slash types
    std::wstring filename = (lastPos != std::wstring::npos) ? str.substr(lastPos + 1) : str;

    // Remove extensions (.exe, .lnk, .url)
    if (filename.size() > 4) {
        std::wstring extension = filename.substr(filename.size() - 4);
        extension = strtowlwr(extension);
        if (extension == L".exe" || extension == L".url" || extension == L".lnk") {
            filename = filename.substr(0, filename.size() - 4);
        }
    }

    return filename;
}

std::wstring getCleanDisplayName(const std::wstring& input) {
    // Remove any path components
    std::wstring result = input;
    size_t lastSlash = result.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        result = result.substr(lastSlash + 1);
    }

    // Remove common file extensions
    std::vector<std::wstring> extensions = {L".lnk", L".exe", L".url", L".bat", L".cmd"};
    for (const auto& ext : extensions) {
        if (result.length() > ext.length()) {
            size_t pos = result.length() - ext.length();
            if (strtowlwr(result.substr(pos)) == ext) {
                result = result.substr(0, pos);
            }
        }
    }

    // Clean up any remaining issues
    result = std::regex_replace(result, std::wregex(L"[\\s]+"), L" "); // Normalize spaces
    result = std::regex_replace(result, std::wregex(L"^\\s+|\\s+$"), L""); // Trim spaces

    // Ensure it's not empty
    if (result.empty()) {
        result = L"Unknown Application";
    }

    return result;
}

// Improved removeWhitespace function
std::wstring removeWhitespace(const std::wstring& str) {
    std::wstring result;
    result.reserve(str.size());

    for (wchar_t c : str) {
        if (!std::iswspace(c)) {
            result.push_back(c);
        }
    }

    return result;
}

// Reset the main window's position and size
void resetWinPos() {
    SetWindowPos(window, nullptr,
                GetSystemMetrics(SM_CXSCREEN) / 2 - 240,
                GetSystemMetrics(SM_CYSCREEN) / 2 - 100,
                480, 32,
                SWP_NOZORDER);

    // Update the rounded corners after resizing
    RECT rect;
    GetWindowRect(window, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    HRGN rgn = CreateRoundRectRgn(0, 0, width+1, height+1, static_cast<int>(edge_radius), static_cast<int>(edge_radius));
    SetWindowRgn(window, rgn, TRUE);
    DeleteObject(rgn);
}

// Clear all search options and reset the window size
void ClearOptions() {
    options.clear();
    resetWinPos();
    InvalidateRect(window, nullptr, TRUE); // Force redraw
}