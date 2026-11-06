#pragma once
#include "Common.h"
#include "DownloadItem.h"
#include "SegmentBar.h"
#include "DownloadEngine.h"

class DownloadPopup {
public:
    static DownloadPopup* Create(HINSTANCE hInstance, HWND hParent, std::shared_ptr<DownloadItem> item, DownloadEngine* engine);
    void Show();
    void UpdateData();
    HWND GetHwnd() const { return m_hWnd; }
    int GetDownloadId() const { return m_item ? m_item->id : 0; }

private:
    DownloadPopup(HINSTANCE hInstance, HWND hParent, std::shared_ptr<DownloadItem> item, DownloadEngine* engine);
    ~DownloadPopup();

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void CreateControls();
    void LayoutControls();
    void ToggleDetails();
    void OnResize(int width, int height);

    HINSTANCE m_hInstance;
    HWND m_hParent;
    HWND m_hWnd = NULL;
    std::shared_ptr<DownloadItem> m_item;
    DownloadEngine* m_engine;

    // Controls
    HWND m_hTab = NULL;
    HWND m_hUrlText = NULL;
    HWND m_hStatusVal = NULL;
    HWND m_hSizeVal = NULL;
    HWND m_hDownloadedVal = NULL;
    HWND m_hSpeedVal = NULL;
    HWND m_hTimeLeftVal = NULL;
    HWND m_hResumeVal = NULL;
    HWND m_hProgressBar = NULL;
    HWND m_hBtnDetails = NULL;
    HWND m_hBtnPause = NULL;
    HWND m_hBtnCancel = NULL;
    HWND m_hConnHeader = NULL;
    HWND m_hSegmentBar = NULL;
    HWND m_hConnList = NULL;
    void SwitchTab(int tabIndex);

    // Speed Limiter Controls (Tab 1)
    HWND m_hChkSpeedLimit = NULL;
    HWND m_hLblMaxSpeed = NULL;
    HWND m_hEditSpeedLimit = NULL;
    HWND m_hLblSpeedUnit = NULL;
    HWND m_hBtnApplySpeed = NULL;
    HWND m_hLblSpeedStatus = NULL;
    int m_currentTab = 0;
    std::vector<HWND> m_statusCtrls;

    bool m_detailsVisible = true;
    HFONT m_hFontRegular = NULL;
    HFONT m_hFontBold = NULL;
    HBRUSH m_hBgBrush = NULL;
};
