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

std::wstring removeWhitespace(const std::wstring& str) {
    std::wstring result;
    for (const wchar_t c : str) {
        if (!std::iswspace(c)) {
            result += c;
        }
    }
    return result;
}

// Function to check if a string contains the buffer text
BOOL containsBuffer(const std::wstring& txt) {
    std::wstring tempTxt = txt;
    // Convert to lowercase (ASCII-only to avoid locale issues)
    for (wchar_t& c : tempTxt) {
        c = (c >= L'A' && c <= L'Z') ? (c + 32) : c;
    }
    const std::wstring bufferStr = removeWhitespace(buffer);
    tempTxt = stringManipulator(tempTxt, DELIMITER);
    tempTxt = removeWhitespace(tempTxt);
    return tempTxt.find(bufferStr) != std::wstring::npos;
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
        for (wchar_t& c : extension) {
            c = (c >= L'A' && c <= L'Z') ? (c + 32) : c;
        }
        return (extension == L".exe" || extension == L".url");
    }
    return false;
}

// Function to execute a command
void ExecuteCommand(const std::wstring& command) {
    SHELLEXECUTEINFOW sei = {0};
    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.nShow = SW_SHOWNORMAL;

    // Properly handle the wstring
    if (IsDirectory(command)) {
        std::cout << "directory" << std::endl;
        sei.lpVerb = L"explore"; // Open directory in Explorer
        sei.lpFile = command.c_str();
    }
    else if (IsExecutable(command)) {
        std::cout << "executable" << std::endl;
        sei.lpVerb = L"open"; // Open file with default application
        sei.lpFile = command.c_str();
    }
    else {
        RunExtraCommands(sei);
        return; // Skip ShellExecuteExW since RunExtraCommands handles it
    }

    // Execute the command
    if (!ShellExecuteExW(&sei)) {
        // Use a better error handling mechanism
        DWORD error = GetLastError();
        std::cerr << "Failed to execute command. Error code: " << error << std::endl;
    }
}

// Function to search for installed applications
std::unordered_set<std::wstring> GetInstalledAppPaths() {
    CoInitialize(nullptr); // Initialize COM
    std::vector<std::wstring> Dirs = {
        std::filesystem::path(std::getenv("USERPROFILE")).wstring() + L"\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs",
        std::filesystem::path(std::getenv("USERPROFILE")).wstring() + L"\\Desktop",
        L"C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs"
    };

    std::unordered_set<std::wstring> appPaths;

    for (const auto& shortcutDir : Dirs) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(shortcutDir)) {
            if (entry.path().extension() == L".lnk") {  // Check for .lnk files
                IShellLinkW* psl = nullptr;
                HRESULT hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&psl));

                if (SUCCEEDED(hres) && psl) {
                    IPersistFile* ppf = nullptr;
                    hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));

                    if (SUCCEEDED(hres) && ppf) {
                        std::wstring widePath = entry.path().wstring();
                        hres = ppf->Load(widePath.c_str(), STGM_READ);

                        if (SUCCEEDED(hres)) {
                            WCHAR szTargetPath[MAX_PATH] = { 0 };
                            if (SUCCEEDED(psl->GetPath(szTargetPath, MAX_PATH, nullptr, SLGP_RAWPATH))) { // Use SLGP_RAWPATH
                                std::wstring targetPath = szTargetPath;
                                if (targetPath.ends_with(L".exe")) { // Ensure it's an .exe
                                    appPaths.insert(targetPath);
                                }
                            }
                        }
                        ppf->Release(); // Release IPersistFile
                    }
                    psl->Release(); // Release IShellLinkW
                }
            } else if (entry.path().extension() == L".url") {
                std::wstring urlPath = entry.path().wstring();
                appPaths.insert(urlPath);
            }
        }
    }

    CoUninitialize(); // Clean up COM
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

    std::unordered_set<std::wstring> alreadyIn;

    for (const auto& app : installedApps) {
        if (containsBuffer(app) && !alreadyIn.contains(stringManipulator(app, DELIMITER))) {
            if (app.substr(app.length() - 4) == L".exe" || app.substr(app.length() - 4) == L".url") {
                options.push_back(app);
                alreadyIn.insert(stringManipulator(app, DELIMITER));
                if (++counter >= NUM_OF_FINDS) break;
            } else {
                std::wcout << app.substr(app.length() - 4) << std::endl;
            }
        }
    }

    if (options.empty())
        options.emplace_back(L"Search in google");

    // Instead of creating child windows, just update the options list and redraw the parent window
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

            const auto hOldFont = SelectObject(hdc, hFont);

            // Draw each option
            for (size_t i = 0; i < options.size(); i++) {
                RECT rc = {0, 32 * static_cast<int>(i) + 32, 480, 32 * static_cast<int>(i) + 64};
                std::wstring dt = options[i];
                dt = stringManipulator(dt, DELIMITER);
                dt[0] = std::towupper(dt[0]);

                // Fix: Properly draw wide strings
                if (i == currentIndex) {
                    // Create a rounded rectangle for selection
                    HBRUSH hSelectedBrush = CreateSolidBrush(RGB(
                        static_cast<int>(std::to_integer<int>(sel_color[0])),
                        static_cast<int>(std::to_integer<int>(sel_color[1])),
                        static_cast<int>(std::to_integer<int>(sel_color[2]))
                    ));
                    HPEN hSelectedPen = CreatePen(PS_SOLID, 1, RGB(120, 120, 200));

                    const auto hOldPen = SelectObject(hdc, hSelectedPen);
                    const auto hOldBrush = SelectObject(hdc, hSelectedBrush);

                    // Draw rounded highlight rectangle
                    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 10, 10);

                    // Draw text with slight offset for a shadow effect
                    RECT shadowRc = {rc.left + 1, rc.top + 1, rc.right + 1, rc.bottom + 1};
                    SetTextColor(hdc, RGB(0, 0, 0));
                    DrawTextW(hdc, dt.c_str(), -1, &shadowRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                    // Draw the main text
                    SetTextColor(hdc, RGB(255, 255, 255));
                    DrawTextW(hdc, dt.c_str(), -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                    // Clean up
                    SelectObject(hdc, hOldPen);
                    SelectObject(hdc, hOldBrush);
                    DeleteObject(hSelectedBrush);
                    DeleteObject(hSelectedPen);
                } else {
                    // Regular text for non-selected items
                    SetTextColor(hdc, RGB(200, 200, 200));
                    DrawTextW(hdc, dt.c_str(), -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                }
            }

            // Clean up
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);

            EndPaint(hwnd, &ps);
            return 0;
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
                    if (bufWithoutSpace == L"exit") {
                        exit();
                    }
                    else if (bufWithoutSpace == L"reload") {
                        installedApps = GetInstalledAppPaths();
                        ShowWindow(window, SW_HIDE);
                        isWindowActive = false;
                    }
                    else if (!options.empty()) {
                        ExecuteCommand(options[currentIndex]);
                        ShowWindow(window, SW_HIDE);
                        isWindowActive = false;
                    } else {
                        std::cout << "passing buffer" << std::endl;
                        ExecuteCommand(buffer);
                    }
                }
            }
        } else {
            keyStates[VK_RETURN] = false;
        }

        if (GetAsyncKeyState(VK_TAB) & 0x8000) {
            keyStates[VK_TAB] = true;
            if (!options.empty()) {
                if (options[currentIndex] != L"Search in google") {
                    const int size_needed = WideCharToMultiByte(CP_UTF8, 0, options[currentIndex].c_str(), -1, nullptr, 0, nullptr, nullptr);
                    std::string narrowStr(size_needed, 0);
                    WideCharToMultiByte(CP_UTF8, 0, options[currentIndex].c_str(), -1, &narrowStr[0], size_needed, nullptr, nullptr);

                    LPCSTR result = narrowStr.c_str();
                    SetWindowText(hEdit, result);
                }
            }
        } else {
            keyStates[VK_TAB] = false;
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            keyStates[VK_ESCAPE] = true;
            ShowWindow(window, SW_HIDE);
            isWindowActive = false;
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