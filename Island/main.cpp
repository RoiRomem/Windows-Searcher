#include "Island.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    // Create window
    if (!CreateIslandWindow(hInstance)) {
        std::cerr << "Failed to create window!" << std::endl;
        return 1;
    }

    // Initialize animation time
    g_lastAnimationTime = std::chrono::steady_clock::now();

    // Set up a timer for animations (16ms ≈ 60fps)
    SetTimer(g_hWnd, 1, 16, nullptr);

    hFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, reinterpret_cast<LPCSTR>(L"Segoe UI"));

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        // Check if any fullscreen window is active
        HWND foregroundWnd = GetForegroundWindow();
        if (foregroundWnd && IsWindowFullscreen(foregroundWnd) && foregroundWnd != g_hWnd) {
            if (g_currentState != IslandState::HIDDEN) {
                TransitionTo(IslandState::HIDDEN);
            }
        } else if (g_currentState == IslandState::HIDDEN) {
            TransitionTo(IslandState::COLLAPSED);
        } else if (g_currentState == IslandState::EXPANDED && !IsMouseOverWindow(g_hWnd)) {
            TransitionTo(IslandState::COLLAPSED);
        }

        // Process message
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

BOOL CreateIslandWindow(HINSTANCE hInstance) {
    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.hbrBackground = CreateSolidBrush(g_backgroundColor.ToColorRef());
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = APP_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassEx(&wc)) {
        return FALSE;
    }

    // Calculate initial position
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int posX = (screenWidth - ISLAND_COLLAPSED_WIDTH) / 2;
    int posY = -ISLAND_HEIGHT * 3 / 4;

    // Create window with layered style for transparency
    g_hWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        APP_NAME,
        APP_NAME,
        WS_POPUP,
        posX, posY,
        ISLAND_COLLAPSED_WIDTH, ISLAND_HEIGHT,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!g_hWnd) {
        return FALSE;
    }

    // Set the window transparency
    SetLayeredWindowAttributes(g_hWnd, 0, g_backgroundColor.a, LWA_ALPHA);

    // Set initial window shape
    UpdateWindowShape();

    // Show the window
    ShowWindow(g_hWnd, SW_SHOWNORMAL);

    UpdateWindow(g_hWnd);

    return TRUE;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Create a memory DC
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdc, ps.rcPaint.right, ps.rcPaint.bottom);
            HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDC, hBitmap));

            // Draw everything on memory DC
            RenderIsland(memDC);

            // Copy from memory DC to window
            BitBlt(hdc, 0, 0, ps.rcPaint.right, ps.rcPaint.bottom, memDC, 0, 0, SRCCOPY);

            // Cleanup
            SelectObject(memDC, oldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(memDC);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_TIMER:
            if (wParam == 1) { // Animation timer
                ProcessAnimations();
                RedrawWindow(g_hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW); // request a rerender
            }
        return 0;

        case WM_MOUSEMOVE: {
            bool wasMouseOver = g_isMouseOver;
            g_isMouseOver = true;

            // If mouse just entered, track when it leaves
            if (!wasMouseOver) {
                TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT)};
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);

                // If in collapsed state, expand
                if (g_currentState == IslandState::COLLAPSED) {
                    TransitionTo(IslandState::EXPANDED);
                    SetForegroundWindow(g_hWnd);
                }
            }
            return 0;
        }

        case WM_MOUSELEAVE:
            g_isMouseOver = false;

            // When mouse leaves, collapse if expanded
            if (g_currentState == IslandState::EXPANDED) {
                TransitionTo(IslandState::COLLAPSED);
            }
            return 0;


        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);

        case WM_ERASEBKGND:
            return TRUE; // Prevent background erasure and reduce flickering
    }
}

void UpdateWindowPosition(bool animate) {
    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int targetWidth = 0, targetY = 0;

    // Determine target dimensions based on state
    switch (g_targetState) {
        case IslandState::EXPANDED:
            targetWidth = ISLAND_EXPANDED_WIDTH;
            targetY = -ISLAND_HEIGHT / 4; // Show more of the island
            break;
        case IslandState::COLLAPSED:
            targetWidth = ISLAND_COLLAPSED_WIDTH;
            targetY = -ISLAND_HEIGHT * 3 / 4; // Hide more
            break;
        case IslandState::HIDDEN:
            targetWidth = ISLAND_COLLAPSED_WIDTH;
            targetY = -ISLAND_HEIGHT - 10; // Completely hide
            break;
    }

    const int targetX = (screenWidth - targetWidth) / 2;

    // If animating, interpolate between current and target
    if (animate && g_animationProgress < 1.0f) {
        // Get current position
        RECT currentRect;
        GetWindowRect(g_hWnd, &currentRect);

        int currentWidth = currentRect.right - currentRect.left;
        int currentX = currentRect.left;
        int currentY = currentRect.top;

        // Calculate eased progress
        float easedProgress = EaseOutQuad(g_animationProgress);

        // Interpolate
        int newWidth = currentWidth + static_cast<int>((targetWidth - currentWidth) * easedProgress);
        int newX = currentX + static_cast<int>((targetX - currentX) * easedProgress);
        int newY = currentY + static_cast<int>((targetY - currentY) * easedProgress);

        // Apply position
        SetWindowPos(g_hWnd, nullptr, newX, newY, newWidth, ISLAND_HEIGHT, SWP_NOZORDER);

        // Update shape for new width
        UpdateWindowShape();
    } else {
        // Directly set to target position
        SetWindowPos(g_hWnd, nullptr, targetX, targetY, targetWidth, ISLAND_HEIGHT, SWP_NOZORDER);
        UpdateWindowShape();
    }
}

void UpdateWindowShape() {
    RECT windowRect;
    GetWindowRect(g_hWnd, &windowRect);
    int width = windowRect.right - windowRect.left;

    // Create rounded rectangle region
    HRGN rgn = CreateRoundRectRgn(0, 0, width + 1, ISLAND_HEIGHT + 1, g_cornerRadius, g_cornerRadius);
    SetWindowRgn(g_hWnd, rgn, TRUE);
    DeleteObject(rgn);
}

void ProcessAnimations() {
    // Calculate time delta
    auto currentTime = std::chrono::steady_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - g_lastAnimationTime).count();
    g_lastAnimationTime = currentTime;

    // If we're animating
    if (g_animationProgress < 1.0f) {
        // Update progress
        g_animationProgress += static_cast<float>(deltaTime) / ANIMATION_DURATION;

        // Clamp progress
        if (g_animationProgress > 1.0f) {
            g_animationProgress = 1.0f;
            g_currentState = g_targetState; // Animation complete
        }

        // Update window position with animation
        UpdateWindowPosition(true);

        // Force redraw
        InvalidateRect(g_hWnd, nullptr, TRUE);
    }
}

void TransitionTo(IslandState state) {
    // Only start transition if needed
    if (state != g_currentState && state != g_targetState) {
        g_targetState = state;
        g_animationProgress = 0.0f;
        g_lastAnimationTime = std::chrono::steady_clock::now();
    }
}

// Simple easing function for smoother animations
float EaseOutQuad(float t) {
    return t * (2.0f - t);
}

void RenderIsland(HDC hdc) {
    RECT clientRect;
    GetClientRect(g_hWnd, &clientRect);

    // Fill background
    HBRUSH bgBrush = CreateSolidBrush(g_backgroundColor.ToColorRef());
    FillRect(hdc, &clientRect, bgBrush);
    DeleteObject(bgBrush);

    // Set up text rendering
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));

    // Define text area
    RECT textRect = clientRect;
    textRect.left += 10;
    textRect.top += 10;
    textRect.right -= 10;

    // Create and apply font
    HFONT hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Fira Code");

    HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

    // COLLAPSED STATE - starts from tPercent 82
    if (isInState(IslandState::COLLAPSED)) {
        AddText(g_hWnd, hdc, 10, 82, GetTime().c_str());
        AddText(g_hWnd, hdc, 43, 82, L"RAM: " + GetMemoryUsage());
    }

    // EXPANDED STATE starts from tPercent 30
    if ((g_currentState == IslandState::EXPANDED && g_targetState == IslandState::EXPANDED) || (g_currentState == IslandState::COLLAPSED && g_targetState == IslandState::EXPANDED)) {
        AddText(g_hWnd, hdc, 3, 30, GetTime());
        AddText(g_hWnd, hdc, 80, 30, L"RAM: " + GetMemoryUsage());

        AddText(g_hWnd, hdc, 38, 50, GetMediaInfo());
    }

    // Cleanup
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}