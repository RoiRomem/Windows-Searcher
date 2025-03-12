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
#include <coroutine>


// Global Variables
inline HWND window = nullptr;
inline HWND hEdit = nullptr;
inline bool isWindowActive = false;
inline char buffer[256] = "";

inline std::unordered_set<std::string> installedApps;

inline std::vector<std::string> options;
inline std::vector<HWND> optionWindows;
inline unsigned int currentIndex = 0;

inline std::atomic<bool> isSearching = true;

inline std::unordered_map<std::string, std::pair<std::wstring, std::wstring>> commandMap;

// Constants
#define APP_NAME "Searcher"
#define MOD_KEY MOD_CONTROL
#define ACT_KEY VK_SPACE
#define ACTION_ID 101
#define EDIT_ID 102
#define NUM_OF_FINDS 4
#define DELIMITER '\\'

// Function Declarations
void CleanUp();
BOOL Create(DWORD dwExStyle, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPSTR lpCmdLine);
void ActivateWindow();
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Search();
void ReactToOptions();
void resetWinPos();
void UpdateBuffer();
BOOL containsBuffer(const std::string &txt);

std::unordered_set<std::string> GetInstalledAppPaths();
void ExecuteCommand(const std::string &command);

std::vector<HWND> OptionDestroyer();
std::string stringManipulator(const std::string& str, const char delimiter);
std::string removeWhitespace(const std::string& str);

void RunExtraCommands(SHELLEXECUTEINFOW &sei);
void OpenGoogle(SHELLEXECUTEINFOW &sei);
void SetCommands();



#endif // SEARCHER_H