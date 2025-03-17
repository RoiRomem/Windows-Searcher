#include "Searcher.h"

// Function to update the buffer with text from the edit control
void UpdateBuffer() {
    // Fix: Properly handle wide string buffer
    GetWindowTextW(hEdit, buffer, sizeof(buffer)/sizeof(wchar_t));

    // Convert to lowercase (ASCII-only to avoid locale issues)
    int length = lstrlenW(buffer);
    for (int i = 0; i < length; ++i) {
        buffer[i] = (buffer[i] >= L'A' && buffer[i] <= L'Z') ? (buffer[i] + 32) : buffer[i];
    }

    currentIndex = 0;
    if (lstrlenW(buffer) > 0)
        Search();
    else
        ClearOptions();
}

// Function to check if a string contains the buffer text
BOOL containsBuffer(const std::wstring& txt) {
    if (txt.empty() || wcslen(buffer) == 0) {
        return FALSE;
    }

    // Get the clean display name for the text
    std::wstring displayName = stringManipulator(txt, DELIMITER);

    // Convert both to lowercase for case-insensitive comparison
    std::wstring lowerDisplayName = strtowlwr(displayName);
    std::wstring lowerBuffer = strtowlwr(removeWhitespace(buffer));

    return lowerDisplayName.find(lowerBuffer) != std::wstring::npos;
}

// Function to check if a path is a directory
bool IsDirectory(const std::wstring& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
}

// Function to check if a path is an executable
bool IsExecutable(const std::wstring& path) {
    if (path.length() >= 4) {
        std::wstring extension = path.substr(path.length() - 4);
        // Convert to lowercase (ASCII-only to avoid locale issues)
        extension = stringManipulator(extension, DELIMITER);
        return (extension == L".exe" || extension == L".url");
    }
    return false;
}

// Function to execute a command
// Fixed ExecuteCommand function with better error handling
void ExecuteCommand(const std::wstring& command) {
    if (command.empty()) {
        std::cerr << "Empty command provided" << std::endl;
        return;
    }

    SHELLEXECUTEINFOW sei = {0};
    sei.cbSize = sizeof(SHELLEXECUTEINFOW);
    sei.nShow = SW_SHOWNORMAL;

    // Determine the appropriate action based on the command type
    if (IsDirectory(command)) {
        sei.lpVerb = L"explore";
        sei.lpFile = command.c_str();
    }
    else if (IsExecutable(command)) {
        sei.lpVerb = L"open";
        sei.lpFile = command.c_str();
    }
    else {
        RunExtraCommands(sei, command);
        return; // RunExtraCommands handles the execution
    }

    // Execute the command with better error handling
    if (!ShellExecuteExW(&sei)) {
        DWORD error = GetLastError();
        std::cerr << "Failed to execute command. Error code: " << error << std::endl;

        // Provide more detailed error information
        LPWSTR messageBuffer = nullptr;
        FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPWSTR>(&messageBuffer),
            0,
            nullptr
        );

        if (messageBuffer) {
            std::wcerr << L"Error message: " << messageBuffer << std::endl;
            LocalFree(messageBuffer);
        }
    }
}

// Function to search for installed applications
std::unordered_map<std::wstring, std::wstring> GetInstalledAppPaths() {
    CoInitialize(nullptr); // Initialize COM
    std::vector<std::wstring> Dirs = {
        std::filesystem::path(std::getenv("USERPROFILE")).wstring() + L"\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs",
        std::filesystem::path(std::getenv("USERPROFILE")).wstring() + L"\\Desktop",
        L"C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs"
    };

    std::unordered_map<std::wstring, std::wstring> appPaths;

    try {
        for (const auto& shortcutDir : Dirs) {
            if (!std::filesystem::exists(shortcutDir)) {
                continue; // Skip non-existent directories
            }

            for (const auto& entry : std::filesystem::recursive_directory_iterator(shortcutDir)) {
                try {
                    std::wstring entryPath = entry.path().wstring();
                    std::wstring extension = entry.path().extension().wstring();

                    // Convert extension to lowercase
                    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);

                    if (extension == L".lnk") {
                        IShellLinkW* psl = nullptr;
                        HRESULT hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                                        IID_IShellLinkW, reinterpret_cast<void**>(&psl));

                        if (SUCCEEDED(hres) && psl) {
                            IPersistFile* ppf = nullptr;
                            hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));

                            if (SUCCEEDED(hres) && ppf) {
                                hres = ppf->Load(entryPath.c_str(), STGM_READ);

                                if (SUCCEEDED(hres)) {
                                    WCHAR szTargetPath[MAX_PATH] = { 0 };
                                    WCHAR szDescription[MAX_PATH] = { 0 };

                                    // Get the target path
                                    if (SUCCEEDED(psl->GetPath(szTargetPath, MAX_PATH, nullptr, SLGP_RAWPATH))) {
                                        std::wstring targetPath = szTargetPath;

                                        // Get the description
                                        psl->GetDescription(szDescription, MAX_PATH);
                                        std::wstring description = szDescription;

                                        // Get the filename without extension
                                        std::wstring filename = entry.path().stem().wstring();

                                        // Choose the best display name (prioritize description, then filename)
                                        std::wstring displayName = !description.empty() ? description : filename;

                                        // Only add if it's an executable
                                        if (IsExecutable(targetPath)) {
                                            appPaths[targetPath] = displayName;
                                        }
                                    }
                                }
                                ppf->Release();
                            }
                            psl->Release();
                        }
                    } else if (extension == L".url") {
                        // For URL files, just use the filename without extension
                        std::wstring displayName = entry.path().stem().wstring();
                        appPaths[entryPath] = displayName;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error processing entry: " << e.what() << std::endl;
                    continue;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in GetInstalledAppPaths: " << e.what() << std::endl;
    }

    CoUninitialize();
    return appPaths;
}

// Function to perform the search
void Search() {
    options.clear();

    int counter = 0;

    // First, search in commandMap
    for (const auto& [cmdName, cmdValue] : commandMap) {
        if (containsBuffer(cmdName)) {
            // Use the command name as both key and display value
            options[cmdName] = cmdName;
            if (++counter >= NUM_OF_FINDS) break;
        }
    }

    // Track which apps we've already added to avoid duplicates
    std::unordered_set<std::wstring> alreadyIn;

    // Then search in installed apps
    for (const auto& [appPath, displayName] : installedApps) {
        // Extract display name for comparison
        std::wstring cleanDisplayName = stringManipulator(displayName, DELIMITER);

        // Create a unique key for checking duplicates
        std::wstring uniqueKey = strtowlwr(cleanDisplayName);

        if ((containsBuffer(appPath) || containsBuffer(displayName)) && !alreadyIn.contains(uniqueKey)) {
            if (IsExecutable(appPath) || appPath.substr(appPath.length() - 4) == L".url") {
                // Store the app path as key and display name as value
                options[appPath] = displayName;
                alreadyIn.insert(uniqueKey);
                if (++counter >= NUM_OF_FINDS) break;
            }
        }
    }

    // If no results, add "Search in google" option
    if (options.empty())
        options[L"Search in google"] = L"Search in google";

    // Update the window size and redraw
    UpdateWindowSize();
    InvalidateRect(window, nullptr, TRUE);
}

// Function to update the window size based on number of options
void UpdateWindowSize() {
    if (!options.empty()) {
        SetWindowPos(window, nullptr,
                    GetSystemMetrics(SM_CXSCREEN) / 2 - 240,
                    GetSystemMetrics(SM_CYSCREEN) / 2 - 100,
                    480, 32 * static_cast<int>(options.size()) + 32,
                    SWP_NOZORDER);

        // Update the region for rounded corners after resizing
        RECT rect;
        GetWindowRect(window, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        HRGN rgn = CreateRoundRectRgn(0, 0, width+1, height+1, static_cast<int>(edge_radius), static_cast<int>(edge_radius));
        SetWindowRgn(window, rgn, TRUE);
        DeleteObject(rgn);
    }
}

// Function to activate the window
void ActivateWindow() {
    ShowWindow(window, SW_SHOW);
    SetForegroundWindow(window);
    if (!hEdit) {
        // Use CreateWindowExW for Unicode support
        hEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 480, 32, window, reinterpret_cast<HMENU>(EDIT_ID), GetModuleHandle(nullptr), nullptr);
        if (HFONT hFont = CreateFont(32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, font.c_str())) {
            SendMessage(hEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        }
    }
    SetWindowTextW(hEdit, L""); // Use SetWindowTextW for wide strings
    resetWinPos();
    SetFocus(hEdit);
    UpdateWindow(window);
}

// Function to clean up resources
void CleanUp() {
    UnregisterHotKey(window, ACTION_ID);
}

void HideWindow() {
    ShowWindow(window, SW_HIDE);
    SetWindowText(hEdit, nullptr);
    options.clear();
    isWindowActive = false;
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

    // Create window with layered style for transparency
    window = CreateWindowEx(dwExStyle | WS_EX_LAYERED, lpWindowName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpCmdLine);

    // Set the window transparency (225 = slight transparency)
    SetLayeredWindowAttributes(window, 0, static_cast<int>(bk_color[3]), LWA_ALPHA);

    // Create rounded corners
    const HRGN rgn = CreateRoundRectRgn(0, 0, nWidth+1, nHeight+1, static_cast<int>(edge_radius), static_cast<int>(edge_radius));
    SetWindowRgn(window, rgn, TRUE);
    DeleteObject(rgn);

    // Set the window background color
    SetClassLongPtr(window, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(CreateSolidBrush(RGB(bk_color[0], bk_color[1], bk_color[2]))));

    return (window ? TRUE : FALSE);
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_HOTKEY:
            if (isWindowActive) {
                HideWindow();
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
                SetTextColor(reinterpret_cast<HDC>(wParam), RGB(txt_color[0], txt_color[1], txt_color[2])); // White text
                SetBkColor(reinterpret_cast<HDC>(wParam), RGB(bk_color[0], bk_color[1], bk_color[2])); // Dark background
                return reinterpret_cast<LRESULT>(CreateSolidBrush(RGB(bk_color[0], bk_color[1], bk_color[2])));
            }
            break;

        // Custom painting for the entire window
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Set up transparent background
            SetBkMode(hdc, TRANSPARENT);

            // Create font for options
            HFONT hFont = CreateFontW(28, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                      OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                      DEFAULT_PITCH | FF_DONTCARE, reinterpret_cast<LPCWSTR>(font.c_str()));

            HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

            // Draw each option
            int i = 0;
            for (const auto &[appPath, displayName]: options) {
                RECT rc = {10, 32 * i + 32, 470, 32 * i + 64};

                // Make sure we're displaying a clean name, never a path
                std::wstring displayText = getCleanDisplayName(displayName);

                // Ensure the first character is uppercase
                if (!displayText.empty() && std::iswlower(displayText[0])) {
                    displayText[0] = std::towupper(displayText[0]);
                }

                // Draw with appropriate styles
                if (i == currentIndex) {
                    // Create a rounded rectangle for selection
                    HBRUSH hSelectedBrush = CreateSolidBrush(RGB(
                        static_cast<int>(std::to_integer<int>(sel_color[0])),
                        static_cast<int>(std::to_integer<int>(sel_color[1])),
                        static_cast<int>(std::to_integer<int>(sel_color[2]))
                    ));
                    HPEN hSelectedPen = CreatePen(PS_SOLID, 1, RGB(120, 120, 200));

                    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hSelectedPen));
                    HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hSelectedBrush));

                    // Draw rounded highlight rectangle
                    RoundRect(hdc, rc.left - 10, rc.top, rc.right, rc.bottom, 10, 10);

                    // Draw text with slight offset for a shadow effect
                    RECT shadowRc = {rc.left + 1, rc.top + 1, rc.right + 1, rc.bottom + 1};
                    SetTextColor(hdc, RGB(0, 0, 0));
                    DrawTextW(hdc, displayText.c_str(), -1, &shadowRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                    // Draw the main text
                    SetTextColor(hdc, RGB(255, 255, 255));
                    DrawTextW(hdc, displayText.c_str(), -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                    // Clean up
                    SelectObject(hdc, hOldPen);
                    SelectObject(hdc, hOldBrush);
                    DeleteObject(hSelectedBrush);
                    DeleteObject(hSelectedPen);
                } else {
                    // Regular text for non-selected items
                    SetTextColor(hdc, RGB(200, 200, 200));
                    DrawTextW(hdc, displayText.c_str(), -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                }
                i++;
            }

            // Clean up
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ACTIVATE:
            if (wParam == WA_INACTIVE) {
                HideWindow();
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

std::wstring keyAtPos(const unsigned int pos) {
    int i = 0;
    for (const auto &key: options | std::views::keys) {
        if (i == pos) return key;
        i++;
    }
    return L"";  // Return empty string if position is out of range
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    // Set locale to "C" to avoid locale-specific behavior
    std::setlocale(LC_ALL, "C");

    SetValues();
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
    installedApps = GetInstalledAppPaths();
    MSG msg = {nullptr};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        // Handle keydown events
        if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
            if (!keyStates[VK_RETURN]) {
                keyStates[VK_RETURN] = true;
                if (lstrlenW(buffer) > 0) {
                    // Fix: Proper string comparison
                    std::wstring bufWithoutSpace = removeWhitespace(buffer);
                    if (keyAtPos(0) == L"exit") {
                        exit();
                    }
                    else if (keyAtPos(0) == L"reload") {
                        installedApps = GetInstalledAppPaths();
                        ShowWindow(window, SW_HIDE);
                        isWindowActive = false;
                    }
                    else { //pizza alfa
                        ExecuteCommand(keyAtPos(currentIndex));
                        ShowWindow(window, SW_HIDE);
                        isWindowActive = false;
                    }
                }
            }
        } else {
            keyStates[VK_RETURN] = false;
        }

        if (GetAsyncKeyState(VK_TAB) & 0x8000) {
            keyStates[VK_TAB] = true;
            if (!options.empty()) {
                if (keyAtPos(currentIndex) != L"Search in google") {
                    const int size_needed = WideCharToMultiByte(CP_UTF8, 0, keyAtPos(currentIndex).c_str(), -1, nullptr, 0, nullptr, nullptr);
                    std::string narrowStr(size_needed, 0);
                    WideCharToMultiByte(CP_UTF8, 0, keyAtPos(currentIndex).c_str(), -1, &narrowStr[0], size_needed, nullptr, nullptr);

                    LPCSTR result = narrowStr.c_str();
                    SetWindowText(hEdit, result);
                }
            }
        } else {
            keyStates[VK_TAB] = false;
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            keyStates[VK_ESCAPE] = true;
            HideWindow();
        } else
            keyStates[VK_ESCAPE] = false;

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

        if (GetAsyncKeyState(VK_UP) & 0x8000) {
            if (!keyStates[VK_UP]) {
                keyStates[VK_UP] = true;
                if (!options.empty()) {
                    if (currentIndex == 0) currentIndex = options.size() - 1;
                    else currentIndex--;
                    InvalidateRect(window, nullptr, TRUE); // Redraw window to update selection
                }
            }
        } else {
            keyStates[VK_UP] = false;
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