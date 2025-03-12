#include "Searcher.h"

std::string stringManipulator(const std::string& str, const char delimiter) {
    std::string temper = str;

    const std::string exe = ".exe";
    const std::string url = ".url";

    // Remove the parts before the filename
    if (const size_t lastPos = temper.find_last_of(delimiter); lastPos != std::string::npos) {
        temper = temper.substr(lastPos + 1);
    }

    // Remove extension properly
    if (temper.size() > 4) { // Ensure it's long enough for an extension
        if (temper.length() >= 4 && temper.compare(temper.length() - 4, 4, exe) == 0) {
            temper.erase(temper.length() - 4);
        } else if (temper.length() >= 4 && temper.compare(temper.length() - 4, 4, url) == 0) {
            temper.erase(temper.length() - 4);
        }
    }

    return temper;
}

// Function to react to search results
void ReactToOptions() {
    // Clear existing windows first
    if (OptionDestroyer().size() == options.size()) {
        for (byte i = 0; i < optionWindows.size(); i++) {
            SendMessage(optionWindows[i], WM_SETTEXT, 0, reinterpret_cast<LPARAM>(options[i].c_str()));
        }
    } else {
        // Create new windows for each option
        for (int i = 0; i < options.size(); ++i) {
            HINSTANCE hInstance = GetModuleHandle(nullptr);
            optionWindows.push_back(CreateWindowEx(
                0, "STATIC", stringManipulator(options[i], DELIMITER).c_str(),
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                0, 32 * i + 32, 480, 32,
                window, nullptr, hInstance, nullptr
            ));
            if (HFONT hFont = CreateFont(32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI")) {
                SendMessage(optionWindows[i], WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
            }
        }

        // Resize the main window to fit all options
        if (!options.empty()) {
            const int totalHeight = 32 * static_cast<int>(options.size()) + 32;
            SetWindowPos(window, nullptr,
                GetSystemMetrics(SM_CXSCREEN) / 2 - 240,
                GetSystemMetrics(SM_CYSCREEN) / 2 - 8,
                480, totalHeight,
                SWP_NOMOVE | SWP_NOZORDER);
        }
    }


    UpdateWindow(window);
}

// Function to reset the window position
void resetWinPos() {
    SetWindowPos(window, nullptr,
        GetSystemMetrics(SM_CXSCREEN) / 2 - 240,
        GetSystemMetrics(SM_CYSCREEN) / 2 - 100,
        480, 32,
        SWP_NOMOVE | SWP_NOZORDER);
}

// Function to destroy option windows
std::vector<HWND> OptionDestroyer() {
    for (HWND hwnd : optionWindows) {
        if (IsWindow(hwnd)) {
            DestroyWindow(hwnd);
        }
    }
    std::vector<HWND> legacyOptions = optionWindows;
    optionWindows.clear();
    resetWinPos();
    InvalidateRect(window, nullptr, TRUE); // Force redraw
    return legacyOptions;
}