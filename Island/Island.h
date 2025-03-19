#ifndef ISLAND_H
#define ISLAND_H

// includes
#include <cstdint>
#include <Windows.h>
#include <iostream>
#include <chrono>
#include <functional>
#include <vector>
#include <memory>
#include <string>
#include <gdiplus.h>

#pragma comment (lib,"Gdiplus.lib")

// defines
#define APP_NAME "Island"
#define ISLAND_COLLAPSED_WIDTH 120
#define ISLAND_EXPANDED_WIDTH 300
#define ISLAND_HEIGHT 150

// Animation durations in milliseconds
#define ANIMATION_DURATION 300

// Graphics values
#define PLAY_PATH L"assets/next.png"
#define NEXT_PATH L"assets/next.png"
#define PREV_PATH L"assets/prev.png"

// Notification structure
struct IslandNotification {
    std::wstring title;
    std::wstring message;
    std::chrono::steady_clock::time_point timestamp;
    std::function<void()> onClickCallback;
    bool isActive = true;
};

// Island state
enum class IslandState {
    COLLAPSED,
    EXPANDED,
    HIDDEN
};

// Color structure (using uint8_t rather than std::byte for simplicity)
struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    COLORREF ToColorRef() const {
        return RGB(r, g, b);
    }
};

// globals
inline HWND g_hWnd = nullptr;
inline Color g_backgroundColor = {0, 0, 0, 225};
inline int g_cornerRadius = 30;
inline IslandState g_currentState = IslandState::COLLAPSED;
inline bool g_isMouseOver = false;
inline std::vector<IslandNotification> g_notifications;
inline std::chrono::steady_clock::time_point g_lastAnimationTime;

// Animation variables
inline float g_animationProgress = 0.0f;
inline IslandState g_targetState = IslandState::COLLAPSED;

// Fonts
inline HFONT hFont = nullptr;

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CreateIslandWindow(HINSTANCE hInstance);
void UpdateWindowPosition(bool animate = true);
void UpdateWindowShape();
bool IsWindowFullscreen(HWND hwnd);
bool IsMouseOverWindow(HWND hwnd);
void ProcessAnimations();
void AddNotification(const std::wstring& title, const std::wstring& message, std::function<void()> callback = nullptr);
void RenderIsland(HDC hdc);
float EaseOutQuad(float t);
void TransitionTo(IslandState state);

void AddText(const HWND hwnd, HDC hdc, int lPercent, int tPercent, const std::wstring& text);
std::wstring GetTime();
BOOL isInState(const IslandState s);
std::wstring GetMemoryUsage();
void DrawButton(const HDC hdc, const LPCWSTR imagePath, const int x, const int y, const int width, const int height);

std::wstring GetMediaInfo();

#endif //ISLAND_H