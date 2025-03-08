#include <ranges>

#include "Searcher.h"

// Function to update the buffer with text from the edit control
void UpdateBuffer() {
    GetWindowText(hEdit, buffer, sizeof(buffer));
    int length = static_cast<int>(strlen(buffer));
    for (int i = 0; i < length; ++i) {
        buffer[i] = static_cast<char>(std::tolower(buffer[i]));
    }
    currentIndex = 0;
    if (strlen(buffer) > 1)
        Search();
    else
        OptionDestroyer();
}

std::string removeWhitespace(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (!std::isspace(c)) {
            result += c;
        }
    }
    return result;
}

// Function to check if a string contains the buffer text
BOOL containsBuffer(const std::string& txt) {
    std::string tempTxt = txt;
    std::ranges::transform(tempTxt, tempTxt.begin(), ::tolower);
    const std::string bufferStr = removeWhitespace(buffer);
    tempTxt = stringManipulator(tempTxt, DELIMITER);
    tempTxt = removeWhitespace(tempTxt);
    return tempTxt.find(bufferStr) != std::string::npos;
}

// Function to check if a path is a directory
bool IsDirectory(const std::string& path) {
    DWORD attributes = GetFileAttributes(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
}

// Function to check if a path is an executable
bool IsExecutable(const std::string& path) {
    if (path.length() >= 4) {
        std::string extension = path.substr(path.length() - 4);
        std::ranges::transform(extension, extension.begin(), ::tolower);
        return (extension == ".exe");
    }
    return false;
}

// Function to execute a command
void ExecuteCommand(const std::string& command) {
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.lpFile = command.c_str();
    sei.nShow = SW_SHOWNORMAL;


    std::cout << "hello world" << std::endl;
    // Determine the appropriate verb
    if (IsDirectory(command)) {
        std::cout << "directory" << std::endl;
        sei.lpVerb = "explore"; // Open directory in Explorer
    }
    else if (IsExecutable(command)) {
        std::cout << "executable" << std::endl;
        sei.lpVerb = "open"; // Open file with default application
    }
    else {
        std::cout << "command" << std::endl;
        RunExtraCommands(sei);
    }



    // Execute the command
    if (!ShellExecuteEx(&sei)) {
        std::cerr << "Failed to execute command: " << command << std::endl;
    }
}

// Function to search for installed applications
std::vector<std::string> GetInstalledAppPaths() {
    const std::string shortcutDir = std::string(getenv("USERPROFILE")) +
        R"(\AppData\Roaming\Microsoft\Windows\Start Menu\Programs)";
    std::vector<std::string> appPaths;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(shortcutDir)) {
        if (entry.path().extension() == ".lnk") {
            IShellLink* psl;
            HRESULT hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<LPVOID *>(&psl));

            if (SUCCEEDED(hres)) {
                IPersistFile* ppf;
                hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID *>(&ppf));

                if (SUCCEEDED(hres)) {
                    std::wstring widePath = entry.path().wstring();
                    hres = ppf->Load(widePath.c_str(), STGM_READ);

                    if (SUCCEEDED(hres)) {
                        TCHAR szTargetPath[MAX_PATH];
                        if (SUCCEEDED(psl->GetPath(szTargetPath, MAX_PATH, nullptr, SLGP_UNCPRIORITY))) {
                            appPaths.emplace_back(szTargetPath);
                        }
                    }
                    ppf->Release();
                }
                psl->Release();
            }
        } else if (entry.path().extension() == ".url") {
            appPaths.emplace_back(entry.path().string()); // I wanna load my steam games alright
        }
    }
    return appPaths;
}

// Function to perform the search
void Search() {
    options.clear();


    int counter = 0;
    for (const auto &cmdName: commandMap | std::views::keys) {
        if (containsBuffer(cmdName)) {
            options.push_back(cmdName);
            if (++counter >= NUM_OF_FINDS) break;
        }
    }

    for (const auto& app : installedApps) {
        if (containsBuffer(app)) {
            options.push_back(app);
            if (++counter >= NUM_OF_FINDS) break;
        }
    }

    ReactToOptions();
}

// Function to activate the window
void ActivateWindow() {
    ShowWindow(window, SW_SHOW);
    SetForegroundWindow(window);
    if (!hEdit) {
        hEdit = CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 480, 32, window, reinterpret_cast<HMENU>(EDIT_ID), GetModuleHandle(nullptr), nullptr);
        if (HFONT hFont = CreateFont(32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI")) {
            SendMessage(hEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        }
    }
    SetWindowText(hEdit, "");
    resetWinPos();
    SetFocus(hEdit);
    UpdateWindow(window);
}

// Function to clean up resources
void CleanUp() {
    UnregisterHotKey(window, ACTION_ID);
}

// Function to create the main window
BOOL Create(DWORD dwExStyle, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPSTR lpCmdLine) {
    WNDCLASS wc = {0};
    wc.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = lpWindowName;
    RegisterClass(&wc);
    window = CreateWindowEx(dwExStyle, lpWindowName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpCmdLine);

    SetClassLongPtr(window, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(CreateSolidBrush(RGB(0, 0, 0))));

    return (window ? TRUE : FALSE);
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_HOTKEY:
            if (isWindowActive) {
                ShowWindow(hwnd, SW_HIDE);
                isWindowActive = false;
            } else {
                ActivateWindow();
                isWindowActive = true;
            }
            return 0;

        case WM_COMMAND:
            if (HIWORD(wParam) == EN_CHANGE) {
                UpdateBuffer();
            }
            return 0;

        case WM_CTLCOLOREDIT:
            if (reinterpret_cast<HWND>(lParam) == hEdit) {
                SetTextColor(reinterpret_cast<HDC>(wParam), RGB(255, 255, 255)); // White text
                SetBkColor(reinterpret_cast<HDC>(wParam), RGB(0, 0, 0)); // Black background
                return reinterpret_cast<LRESULT>(CreateSolidBrush(RGB(0, 0, 0)));
            }
            break;

        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            HWND hwndStatic = reinterpret_cast<HWND>(lParam);

            // Highlight current index
            if (hwndStatic == optionWindows[currentIndex]) {
                SetTextColor(hdc, RGB(0, 0, 0)); // Black text
                SetBkColor(hdc, RGB(255, 255, 255)); // White background
                return reinterpret_cast<LRESULT>(CreateSolidBrush(RGB(255, 255, 255)));
            }
            // Default for other options
            else {
                SetTextColor(hdc, RGB(255, 255, 255)); // White text
                SetBkColor(hdc, RGB(0, 0, 0)); // Black background
                return reinterpret_cast<LRESULT>(CreateSolidBrush(RGB(0, 0, 0)));
            }
        }

        case WM_ACTIVATE:
            if (wParam == WA_INACTIVE) {
                ShowWindow(hwnd, SW_HIDE);
                isWindowActive = false;
            }
            break;

        case WM_DESTROY:
            CleanUp();
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Updated exit function to properly close the app
void exit() {
    SendMessage(window, WM_CLOSE, 0, 0);
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    installedApps = GetInstalledAppPaths();
    SetCommands();


    if (!Create(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, APP_NAME, WS_POPUP,
                GetSystemMetrics(SM_CXSCREEN) / 2 - 240,
                GetSystemMetrics(SM_CYSCREEN) / 2 - 100,
                480, 32, nullptr, nullptr, hInstance, nullptr)) {
        std::cerr << "Failed to create window!" << std::endl;
        return 1;
    }

    if (!RegisterHotKey(window, ACTION_ID, MOD_KEY, ACT_KEY)) {
        std::cerr << "Failed to register hotkey!" << std::endl;
        return 1;
    }

    // Key state tracking
    bool keyStates[256] = {false};

    MSG msg = {nullptr};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        // Handle keydown events
        if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
            if (!keyStates[VK_RETURN]) {
                keyStates[VK_RETURN] = true;
                if (strlen(buffer) > 0) {
                    // If the buffer is "exit", then close the app
                    if (removeWhitespace(buffer) == "exit") {
                        exit();
                    } else if (removeWhitespace(buffer) == "firefox") { //added this exception because it kept giving tor browser a priority, and I'm not dailying tor
                        ExecuteCommand(R"(C:\Program Files\Mozilla Firefox\firefox.exe)");
                        ShowWindow(window, SW_HIDE);
                        isWindowActive = false;
                    }
                    else if (removeWhitespace(buffer) == "reload") {
                        installedApps = GetInstalledAppPaths();
                        ShowWindow(window, SW_HIDE);
                        isWindowActive = false;
                    }
                    else if (!options.empty()) {
                        ExecuteCommand(options[currentIndex]);
                        ShowWindow(window, SW_HIDE);
                        isWindowActive = false;
                    } else {
                        ExecuteCommand(buffer);
                    }
                }
            }
        } else {
            keyStates[VK_RETURN] = false;
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            if (!keyStates[VK_ESCAPE]) {
                keyStates[VK_ESCAPE] = true;
                ShowWindow(window, SW_HIDE);
                isWindowActive = false;
            }
        } else {
            keyStates[VK_ESCAPE] = false;
        }

        if (GetAsyncKeyState(VK_UP) & 0x8000) {
            if (!keyStates[VK_UP]) {
                keyStates[VK_UP] = true;
                if (!options.empty()) {
                    if (currentIndex == 0) currentIndex = options.size() - 1;
                    else currentIndex--;
                    InvalidateRect(window, nullptr, TRUE);
                }
            }
        } else {
            keyStates[VK_UP] = false;
        }

        if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
            if (!keyStates[VK_DOWN]) {
                keyStates[VK_DOWN] = true;
                if (!options.empty()) {
                    if (currentIndex == options.size() - 1) currentIndex = 0;
                    else currentIndex++;
                    InvalidateRect(window, nullptr, TRUE); // Redraw window to update selection
                }
            }
        } else {
            keyStates[VK_DOWN] = false;
        }
        bool wasCtrlAPressed = false;
        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState('A') & 0x8000)) {
            SendMessage(hEdit, EM_SETSEL, 0, -1);
            wasCtrlAPressed = true;
        }

        if (std::ranges::all_of(keyStates, [](bool b){ return !b; }) && !wasCtrlAPressed) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}
