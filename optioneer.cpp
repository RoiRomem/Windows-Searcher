#include "Searcher.h"

// Improved version of stringManipulator
std::string stringManipulator(const std::string& str, char delimiter) {
    std::string_view view = str;

    // Extract filename after the last delimiter
    if (size_t lastPos = view.find_last_of(delimiter); lastPos != std::string_view::npos) {
        view.remove_prefix(lastPos + 1);
    }

    // Remove extensions (.exe or .url)
    if (view.size() > 4) {
        if (view.ends_with(".exe") || view.ends_with(".url")) {
            view.remove_suffix(4);
        }
    }

    return std::string(view);
}

// React to search results: update existing or create new option windows
void ReactToOptions() {
    if (optionWindows.size() == options.size()) {
        // Only update existing windows
        for (size_t i = 0; i < optionWindows.size(); i++) {
            if (IsWindow(optionWindows[i])) {
                std::string cleanText = stringManipulator(options[i], DELIMITER); // Ensure correct text
                SendMessage(optionWindows[i], WM_SETTEXT, 0, reinterpret_cast<LPARAM>(cleanText.c_str()));
            }
        }
        return;
    }

    // Only destroy windows if the size is different
    if (optionWindows.size() != options.size()) {
        OptionDestroyer();
    }

    HINSTANCE hInstance = GetModuleHandle(nullptr);
    static HFONT hFont = CreateFont(32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    for (size_t i = 0; i < options.size(); ++i) {
        std::string cleanText = stringManipulator(options[i], DELIMITER); // Ensure correct text
        HWND hwnd = CreateWindowEx(0, "STATIC", cleanText.c_str(),
                                   WS_CHILD | WS_VISIBLE | SS_LEFT,
                                   0, 32 * static_cast<int>(i) + 32, 480, 32,
                                   window, nullptr, hInstance, nullptr);
        if (hwnd) {
            SendMessage(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
            optionWindows.push_back(hwnd);
        }
    }

    // Resize window based on number of options
    if (!options.empty()) {
        SetWindowPos(window, nullptr,
                     GetSystemMetrics(SM_CXSCREEN) / 2 - 240,
                     GetSystemMetrics(SM_CYSCREEN) / 2 - 8,
                     480, 32 * static_cast<int>(options.size()) + 32,
                     SWP_NOZORDER);
    }

    UpdateWindow(window);
}


// Reset the main window's position and size
void resetWinPos() {
    SetWindowPos(window, nullptr,
                 GetSystemMetrics(SM_CXSCREEN) / 2 - 240,
                 GetSystemMetrics(SM_CYSCREEN) / 2 - 100,
                 480, 32,
                 SWP_NOMOVE | SWP_NOZORDER);
}

// Destroy all option windows and return previous handles
std::vector<HWND> OptionDestroyer() {
    std::vector<HWND> legacyOptions = optionWindows; // Avoid move to keep data intact

    for (HWND hwnd : legacyOptions) {
        if (IsWindow(hwnd)) {
            DestroyWindow(hwnd);
        }
    }

    if (!legacyOptions.empty()) {
        optionWindows.clear();
    }

    resetWinPos();
    InvalidateRect(window, nullptr, TRUE); // Force redraw
    return legacyOptions;
}