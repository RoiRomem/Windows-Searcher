#include "Island.h"

#include <locale>
#include <codecvt>

#include <windows.h>
#include <wrl/client.h>  // For Microsoft::WRL::ComPtr
#include <shobjidl.h>    // For the Shell API
#include <propkey.h>     // For PKEY_Title and PKEY_Author
#include <propvarutil.h> // For PropVariantInit and PropVariantClear
#include <mmdeviceapi.h> // For audio session manager and device APIs
#include <audiopolicy.h> // For IAudioSessionManager2
#include <endpointvolume.h> // For IAudioEndpointVolume
#include <mfidl.h>       // For media foundation interfaces
#include <mfobjects.h>   // For MF object types
#include <mfplay.h>      // For media playback functions
#include <mfapi.h>       // For MFStartup and MFShutdown

using namespace Microsoft::WRL; // For ComPtr

bool endsWith(const std::wstring& str, const std::wstring& suffix) {
    if (str.size() >= suffix.size()) {
        return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
    return false;
}

std::wstring CStrToWString(const char* cstr) {
    if (!cstr) return L""; // Handle null input
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(cstr);
}

std::wstring GetTime() {
    const std::time_t now = std::time(nullptr);
    const std::tm* localTime = std::localtime(&now);

    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%H:%M", localTime);
    return CStrToWString(buffer);
}

std::wstring GetMemoryUsage() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (GlobalMemoryStatusEx(&memInfo)) {
        const double usage = (1.0 - (static_cast<double>(memInfo.ullAvailPhys) / memInfo.ullTotalPhys)) * 100;
        return std::to_wstring(static_cast<int>(usage)) + L"%";
    }

    return L"Error retrieving memory usage";
}

std::wstring GetMediaInfo() {
    std::wstring mediaInfo = L"No media playing";

    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        return L"Failed to initialize COM";
    }

    // Create device enumerator
    ComPtr<IMMDeviceEnumerator> deviceEnumerator;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          IID_PPV_ARGS(&deviceEnumerator));
    if (FAILED(hr)) {
        CoUninitialize();
        return L"Failed to create device enumerator";
    }

    // Get default audio endpoint
    ComPtr<IMMDevice> defaultDevice;
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    if (FAILED(hr)) {
        CoUninitialize();
        return L"Failed to get default audio endpoint";
    }

    // Activate audio session manager
    ComPtr<IAudioSessionManager2> sessionManager;
    hr = defaultDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL,
                                nullptr, &sessionManager);
    if (FAILED(hr)) {
        CoUninitialize();
        return L"Failed to activate audio session manager";
    }

    // Get session enumerator
    ComPtr<IAudioSessionEnumerator> sessionEnumerator;
    hr = sessionManager->GetSessionEnumerator(&sessionEnumerator);
    if (FAILED(hr)) {
        CoUninitialize();
        return L"Failed to get session enumerator";
    }

    // Count sessions
    int sessionCount;
    hr = sessionEnumerator->GetCount(&sessionCount);
    if (FAILED(hr)) {
        CoUninitialize();
        return L"Failed to get session count";
    }

    // Iterate through sessions to find active one
    for (int i = 0; i < sessionCount; i++) {
        ComPtr<IAudioSessionControl> sessionControl;
        hr = sessionEnumerator->GetSession(i, &sessionControl);
        if (FAILED(hr)) continue;

        ComPtr<IAudioSessionControl2> sessionControl2;
        hr = sessionControl.As(&sessionControl2);
        if (FAILED(hr)) continue;

        // Check if session is active
        AudioSessionState state;
        hr = sessionControl->GetState(&state);
        if (FAILED(hr) || state != AudioSessionStateActive) continue;

        // Get process ID
        DWORD processId;
        hr = sessionControl2->GetProcessId(&processId);
        if (FAILED(hr)) continue;

        // Get process name
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (hProcess) {
            wchar_t processName[MAX_PATH];
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameW(hProcess, 0, processName, &size)) {
                // Extract filename from path
                wchar_t* fileName = wcsrchr(processName, L'\\');
                if (fileName) {
                    mediaInfo = L"Playing: ";
                    if (endsWith(fileName, L".exe"))
                    {
                        size_t len = wcslen(fileName);
                        fileName[len - 4] = L'\0';
                    }
                    mediaInfo += (fileName + 1);
                }
            }
            CloseHandle(hProcess);
        }
        break;
    }

    CoUninitialize();
    return mediaInfo;
}