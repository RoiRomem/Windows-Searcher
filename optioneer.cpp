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