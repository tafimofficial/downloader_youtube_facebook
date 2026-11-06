#pragma once
#include "Common.h"
#include "DownloadItem.h"

class DownloadTask;

class IDownloadCallback {
public:
    virtual void OnProgress(int downloadId) = 0;
    virtual void OnStatusChange(int downloadId, DownloadStatus status) = 0;
    virtual void OnFinished(int downloadId, bool success, const std::wstring& errorMsg) = 0;
};

class DownloadEngine {
public:
    DownloadEngine();
    ~DownloadEngine();

    void SetCallback(IDownloadCallback* cb) { m_callback = cb; }

    int AddDownload(const std::wstring& url, const std::wstring& saveFolder, const std::wstring& customFilename = L"", const std::wstring& category = L"Programs",
        const std::wstring& userAgent = L"", const std::wstring& cookies = L"", const std::wstring& referer = L"",
        const std::wstring& audioUrl = L"", bool isDash = false, int qualityHeight = 0);
    bool StartDownload(int downloadId);
    bool PauseDownload(int downloadId);
    bool ResumeDownload(int downloadId);
    bool CancelDownload(int downloadId);
    bool DeleteDownload(int downloadId, bool deleteFile = false);
    bool UpdateDownloadUrl(int downloadId, const std::wstring& newUrl);

    std::shared_ptr<DownloadItem> GetItem(int downloadId);
    std::vector<std::shared_ptr<DownloadItem>> GetAllItems();

    void SetGlobalSpeedLimit(bool enabled, uint32_t kbps);
    bool IsGlobalSpeedLimitEnabled() const { return m_globalSpeedLimitEnabled; }
    uint32_t GetGlobalSpeedLimitKBps() const { return m_globalSpeedLimitKBps; }

    void SetDefaultNumConnections(int num) { m_defaultNumConnections = (num >= 1 && num <= 32) ? num : 8; }
    int GetDefaultNumConnections() const { return m_defaultNumConnections; }

    void SaveDownloads();
    void LoadDownloads();

private:
    std::mutex m_mutex;
    std::vector<std::shared_ptr<DownloadItem>> m_items;
    std::vector<std::shared_ptr<DownloadTask>> m_tasks;
    IDownloadCallback* m_callback = nullptr;
    int m_nextId = 1;
    std::wstring m_dataFile;

    bool m_globalSpeedLimitEnabled = false;
    uint32_t m_globalSpeedLimitKBps = 500;
    int m_defaultNumConnections = 8;

    std::shared_ptr<DownloadTask> FindTask(int id);
};
