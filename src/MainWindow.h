#pragma once
#include "Common.h"
#include "DownloadEngine.h"
#include "DownloadPopup.h"
#include <functional>

class MainWindow : public IDownloadCallback {
public:
    static MainWindow* Create(HINSTANCE hInstance, const std::wstring& cmdLine = L"");
    int Run();

    // IDownloadCallback implementation
    virtual void OnProgress(int downloadId) override;
    virtual void OnStatusChange(int downloadId, DownloadStatus status) override;
    virtual void OnFinished(int downloadId, bool success, const std::wstring& errorMsg) override;

    void OpenDownloadPopup(std::shared_ptr<DownloadItem> item);
    void RefreshListView();

    ~MainWindow();

private:
    MainWindow(HINSTANCE hInstance, const std::wstring& cmdLine = L"");

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ModalDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void CreateMenus();
    void CreateToolbar();
    void CreateTreeAndList();
    void CreateStatusBar();
    void PopulateTreeView();
    void FilterListByCategory(const std::wstring& category);
    void OnResize(int width, int height);

    void ShowAddUrlDialog();
    void ShowOptionsDialog();
    void ShowSpeedLimiterDialog();
    void ShowChecksumDialog(HWND hParent, const std::wstring& filePath, const std::wstring& fileName);
    void CheckShutdownCondition();
    void UpdateSpeedLimiterStatusBar();
    void OnAddUrlSubmitted(const std::wstring& url, const std::wstring& filename = L"", const std::wstring& customFolder = L"",
        const std::wstring& userAgent = L"", const std::wstring& cookies = L"", const std::wstring& referer = L"",
        const std::wstring& audioUrl = L"", bool isDash = false, int qualityHeight = 0);
    void OnResumeSelected();
    void OnStopSelected();
    void OnStopAll();
    void OnDeleteSelected();
    void OnRefreshAddress();
    void OnDeleteCompleted();

    // Tray & Background
    void InitTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayBalloon(const std::wstring& title, const std::wstring& msg);
    void ShowContextMenuTray(POINT pt);
    void SetAutoStart(bool enable);

    // Embedded HTTP Server for Browser Extension
    void StartHttpServer();
    void StopHttpServer();
    void HttpServerLoop();

    HINSTANCE m_hInstance;
    HWND m_hWnd = NULL;
    HWND m_hToolbar = NULL;
    HWND m_hTreeView = NULL;
    HWND m_hListView = NULL;
    HWND m_hStatusBar = NULL;

    HFONT m_hFont = NULL;
    std::unique_ptr<DownloadEngine> m_engine;
    std::vector<DownloadPopup*> m_popups;

    int m_splitterX = 180;
    std::wstring m_currentFilter = L"All Downloads";
    std::wstring m_defaultDownloadDir;

    NOTIFYICONDATAW m_nid = { 0 };
    bool m_startMinimized = false;
    bool m_reallyExit = false;

    // Feature settings
    bool m_clipboardMonitoring = true;
    bool m_soundOnFinish = true;
    bool m_shutdownWhenDone = false;
    std::wstring m_lastClipboardUrl;

    // HTTP Server
    std::thread m_httpThread;
    std::atomic<bool> m_httpRunning = false;
    SOCKET m_listenSocket = INVALID_SOCKET;
};
