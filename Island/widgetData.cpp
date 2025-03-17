#include "Island.h"
#include <Windows.h>
#include <wrl.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.ApplicationModel.Core.h>


std::string GetTime() {
    const std::time_t now = std::time(nullptr);
    const std::tm* localTime = std::localtime(&now);

    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%H:%M", localTime);
    return buffer;
}

std::string GetMemoryUsage() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (GlobalMemoryStatusEx(&memInfo)) {
        double usage = (1.0 - (static_cast<double>(memInfo.ullAvailPhys) / memInfo.ullTotalPhys)) * 100;
        return std::to_string(static_cast<int>(usage)) + "%";
    }

    return "Error retrieving memory usage";
}

void initWinRT() {
    winrt::init_apartment();
}

// Get the current media session
winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession getMediaSession() {
    auto sessionManager = winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
    return sessionManager.GetCurrentSession();
}

std::string getTrackTitle() {
    std::string result = "No track playing";

    // This will run the code asynchronously on the UI thread.
    winrt::Windows::ApplicationModel::Core::CoreApplication::MainView().Dispatcher().RunAsync(
        winrt::Windows::UI::Core::CoreDispatcherPriority::Normal,
        [&result]() {
            // Inside the lambda, we can run the async task properly.
            if (auto session = getMediaSession()) {
                auto mediaProps = session.TryGetMediaPropertiesAsync().get();
                result = winrt::to_string(mediaProps.Title());
            }
        }).get();  // Wait for the async task to finish

    return result;
}

std::string getTrackArtist() {
    std::string result = "Unknown Artist";

    // This will run the code asynchronously on the UI thread.
    winrt::Windows::ApplicationModel::Core::CoreApplication::MainView().Dispatcher().RunAsync(
        winrt::Windows::UI::Core::CoreDispatcherPriority::Normal,
        [&result]() {
            // Inside the lambda, we can run the async task properly.
            if (auto session = getMediaSession()) {
                auto mediaProps = session.TryGetMediaPropertiesAsync().get();
                result = winrt::to_string(mediaProps.Artist());
            }
        }).get();  // Wait for the async task to finish

    return result;
}

// Simulate media key press
void simulateMediaKey(WORD key) {
    keybd_event(key, 0, 0, 0);
    keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
}

// Toggle play/pause
void playPause() {
    simulateMediaKey(VK_MEDIA_PLAY_PAUSE);
}

// Skip to next track
void nextTrack() {
    simulateMediaKey(VK_MEDIA_NEXT_TRACK);
}

// Go to previous track
void previousTrack() {
    simulateMediaKey(VK_MEDIA_PREV_TRACK);
}