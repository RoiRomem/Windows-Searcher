#ifndef SEARCHER_H
#define SEARCHER_H

#include <iostream>
#include <Windows.h>
#include <vector>
#include <string>
#include <algorithm>
#include <shellapi.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <ranges>
#include <future>
#include <map>
#include <cwctype>

// Global Variables
inline HWND window = nullptr;
inline HWND hEdit = nullptr;
inline bool isWindowActive = false;
inline wchar_t buffer[256] = L"";

inline std::unordered_set<std::wstring> installedApps;

inline std::vector<std::wstring> options;
inline unsigned int currentIndex = 0;

inline std::unordered_map<std::wstring, std::pair<std::wstring, std::wstring>> commandMap;

// Constants
#define APP_NAME "Searcher"
#define MOD_KEY MOD_CONTROL
#define ACT_KEY VK_SPACE
#define ACTION_ID 101
#define EDIT_ID 102
#define NUM_OF_FINDS 4
#define DELIMITER '\\'
#define CNF_NAME "style.cnf"


// styling configs
inline std::byte edge_radius = std::byte{0};
inline std::string font = std::string();
inline std::byte bk_color[4] = {std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};
inline std::byte txt_color[3] = {std::byte{0}, std::byte{0}, std::byte{0}};
inline std::byte sel_color[3] = {std::byte{0}, std::byte{0}, std::byte{0}};
inline std::byte selo_color[3] = {std::byte{0}, std::byte{0}, std::byte{0}};

// Function Declarations
void CleanUp();
BOOL Create(DWORD dwExStyle, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPSTR lpCmdLine);
void ActivateWindow();
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Search();
void UpdateWindowSize();
void resetWinPos();
void UpdateBuffer();
BOOL containsBuffer(const std::wstring& txt);

std::unordered_set<std::wstring> GetInstalledAppPaths();
void ExecuteCommand(const std::wstring& command);

void ClearOptions();
std::wstring stringManipulator(const std::wstring& str, char delimiter);
std::wstring removeWhitespace(const std::wstring& str);

void RunExtraCommands(SHELLEXECUTEINFOW &sei);
void OpenGoogle(SHELLEXECUTEINFOW &sei);
void SetCommands();

void SetValues();

// Helper function for Windows API issues
bool IsDirectory(const std::wstring& path);

inline std::wstring ConvertToWString(const TCHAR* tcharStr) {
#ifdef UNICODE
    return std::wstring(tcharStr); // Direct conversion
#else
    int len = MultiByteToWideChar(CP_ACP, 0, tcharStr, -1, nullptr, 0);
    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_ACP, 0, tcharStr, -1, &wstr[0], len);
    return wstr;
#endif
}

#endif // SEARCHER_H