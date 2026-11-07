#pragma once
#include "Common.h"

enum class DownloadStatus {
    Queued,
    Connecting,
    Downloading,
    Paused,
    Complete,
    Error,
    Canceled
};

inline std::wstring StatusToString(DownloadStatus st, bool isMerging = false, const std::wstring& desc = L"") {
    if (isMerging) return L"Merging audio and video (DASH)...";
    switch (st) {
    case DownloadStatus::Queued: return L"Queued";
    case DownloadStatus::Connecting: return L"Connecting...";
    case DownloadStatus::Downloading: return L"Receiving data...";
    case DownloadStatus::Paused: return L"Paused";
    case DownloadStatus::Complete: return L"Complete";
    case DownloadStatus::Error:
        if (!desc.empty() && desc.find(L"expired") != std::wstring::npos) {
            return L"Address expired";
        }
        return L"Error";
    case DownloadStatus::Canceled: return L"Canceled";
    default: return L"Unknown";
    }
}

struct ConnectionInfo {
    int id = 0;
    uint64_t startByte = 0;
    uint64_t endByte = 0;
    uint64_t downloadedBytes = 0;
    std::wstring status = L"Connecting...";
};

struct DownloadItem {
    int id = 0;
    std::wstring url;
    std::wstring audioUrl = L"";       // DASH separate audio stream URL
    bool isDash = false;               // True if DASH adaptive stream (separate video + audio)
    std::wstring fileName;
    std::wstring savePath;
    std::wstring category = L"Programs";
    std::wstring description = L"";
    std::wstring userAgent = L"";
    std::wstring cookies = L"";
    std::wstring referer = L"";
    
    uint64_t totalBytes = 0;           // Video stream total bytes
    uint64_t downloadedBytes = 0;      // Video stream downloaded bytes
    
    uint64_t audioTotalBytes = 0;      // Audio stream total bytes
    uint64_t audioDownloadedBytes = 0; // Audio stream downloaded bytes
    bool isMerging = false;            // True when ffmpeg remuxing is running
    std::wstring muxStatus = L"";
    
    double speedBytesPerSec = 0.0;
    double timeLeftSec = 0.0;
    bool resumeCapable = true;
    
    DownloadStatus status = DownloadStatus::Queued;
    std::wstring dateAdded;
    std::wstring lastTryDate;
    
    int numConnections = 8;
    std::vector<ConnectionInfo> connections;
    
    // Speed Limiter
    bool speedLimitEnabled = false;
    uint32_t speedLimitKBps = 500; // in KB/sec

    int qualityHeight = 0; // Target video resolution height (1080, 720, 480, 2160, etc.)

    HWND hwndProgressPopup = NULL; // Image 1 popup window handle if open

    uint64_t GetCombinedTotalBytes() const {
        return totalBytes + (isDash ? audioTotalBytes : 0);
    }

    uint64_t GetCombinedDownloadedBytes() const {
        return downloadedBytes + (isDash ? audioDownloadedBytes : 0);
    }

    double GetPercent() const {
        uint64_t tot = GetCombinedTotalBytes();
        if (tot == 0) return 0.0;
        return ((double)GetCombinedDownloadedBytes() / (double)tot) * 100.0;
    }
};
