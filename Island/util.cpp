#include "Island.h"

bool IsWindowFullscreen(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }

    // Get the window style
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);

    // Typically fullscreen apps don't have these borders
    if ((style & WS_CAPTION) || (style & WS_THICKFRAME)) {
        return false;
    }

    // Get the window's rectangle
    RECT windowRect;
    if (!GetWindowRect(hwnd, &windowRect)) {
        return false;
    }

    // Get the screen dimensions for the monitor containing this window
    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo = {sizeof(MONITORINFO)};
    if (!GetMonitorInfo(hMonitor, &monitorInfo)) {
        return false;
    }

    // Compare window dimensions with monitor dimensions
    return (windowRect.left == monitorInfo.rcMonitor.left &&
            windowRect.right == monitorInfo.rcMonitor.right &&
            windowRect.top == monitorInfo.rcMonitor.top &&
            windowRect.bottom == monitorInfo.rcMonitor.bottom);
}

bool IsMouseOverWindow(HWND hwnd) {
    POINT cursorPos;
    if (!GetCursorPos(&cursorPos)) {
        return false;
    }

    RECT windowRect;
    if (!GetWindowRect(hwnd, &windowRect)) {
        return false;
    }

    return PtInRect(&windowRect, cursorPos);
}

void AddText(const HWND hwnd, HDC hdc, int lPercent, int tPercent, const std::wstring& text) {
    RECT rect;
    GetClientRect(hwnd, &rect);

    const float xp = lPercent / 100.0f;
    const float yp = tPercent / 100.0f;

    int x = static_cast<int>(rect.right * xp);
    int y = static_cast<int>(rect.bottom * yp);

    RECT textRect = {x, y, rect.right, rect.bottom};

    // Use DrawTextW for wide strings
    DrawTextW(hdc, text.c_str(), -1, &textRect, DT_LEFT | DT_TOP);
}


BOOL isInState(const IslandState s) {
    return (s == g_targetState && s == g_currentState) || (s == g_targetState && IslandState::HIDDEN == g_currentState);
}

void DrawButton(const HDC hdc, const LPCWSTR imagePath, const int x, const int y, const int width, const int height) {
    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Image image(imagePath);
    graphics.DrawImage(&image, x, y, width, height);
}