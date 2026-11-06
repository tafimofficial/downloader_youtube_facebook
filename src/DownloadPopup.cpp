#include "DownloadPopup.h"
#include "Resource.h"

#define WC_DOWNLOADPOPUP L"TafimDownloadPopup"

static const int POPUP_WIDTH = 570;
static const int POPUP_HEIGHT_EXPANDED = 520;
static const int POPUP_HEIGHT_COLLAPSED = 270;

DownloadPopup* DownloadPopup::Create(HINSTANCE hInstance, HWND hParent, std::shared_ptr<DownloadItem> item, DownloadEngine* engine) {
    static bool registered = false;
    if (!registered) {
        WNDCLASSEX wc = { 0 };
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = DownloadPopup::WndProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = WC_DOWNLOADPOPUP;
        wc.hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
        wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
        RegisterClassEx(&wc);
        registered = true;
    }

    DownloadPopup* p = new DownloadPopup(hInstance, hParent, item, engine);
    return p;
}

DownloadPopup::DownloadPopup(HINSTANCE hInstance, HWND hParent, std::shared_ptr<DownloadItem> item, DownloadEngine* engine)
    : m_hInstance(hInstance), m_hParent(hParent), m_item(item), m_engine(engine) {

    m_hFontRegular = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    m_hFontBold = CreateFontW(-11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    m_hBgBrush = CreateSolidBrush(RGB(240, 240, 240));

    // Centered or cascaded window position
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    int x = (scrW - POPUP_WIDTH) / 2 + (item ? (item->id * 20 % 100) : 0);
    int y = (scrH - POPUP_HEIGHT_EXPANDED) / 2 + (item ? (item->id * 20 % 100) : 0);

    m_hWnd = CreateWindowExW(
        WS_EX_APPWINDOW | WS_EX_TOPMOST,
        WC_DOWNLOADPOPUP,
        L"0% Tafim Internet Download Manager",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        x, y, POPUP_WIDTH, POPUP_HEIGHT_EXPANDED,
        NULL, // Top-level window so it's never hidden by tray main window
        NULL,
        hInstance,
        this
    );

    HICON hBigIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    HICON hSmallIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    if (hBigIcon) SendMessageW(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hBigIcon);
    if (hSmallIcon) SendMessageW(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hSmallIcon);

    if (m_item) {
        m_item->hwndProgressPopup = m_hWnd;
    }
}

DownloadPopup::~DownloadPopup() {
    if (m_item && m_item->hwndProgressPopup == m_hWnd) {
        m_item->hwndProgressPopup = NULL;
    }
    if (m_hFontRegular) DeleteObject(m_hFontRegular);
    if (m_hFontBold) DeleteObject(m_hFontBold);
    if (m_hBgBrush) DeleteObject(m_hBgBrush);
}

void DownloadPopup::Show() {
    if (m_hWnd) {
        ShowWindow(m_hWnd, SW_SHOWNORMAL);
        UpdateWindow(m_hWnd);

        // Bring popup window directly to the front of Chrome / any active browser
        HWND hFore = GetForegroundWindow();
        DWORD foreThread = hFore ? GetWindowThreadProcessId(hFore, NULL) : 0;
        DWORD appThread = GetCurrentThreadId();
        if (foreThread && foreThread != appThread) {
            AttachThreadInput(foreThread, appThread, TRUE);
            SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            SetForegroundWindow(m_hWnd);
            BringWindowToTop(m_hWnd);
            AttachThreadInput(foreThread, appThread, FALSE);
        } else {
            SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            SetForegroundWindow(m_hWnd);
            BringWindowToTop(m_hWnd);
        }
        UpdateData();
    }
}

void DownloadPopup::CreateControls() {
    // 1. Top Tab Control
    m_hTab = CreateWindowExW(0, WC_TABCONTROL, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        10, 10, POPUP_WIDTH - 36, 26,
        m_hWnd, (HMENU)IDC_POPUP_TAB, m_hInstance, NULL);
    SendMessage(m_hTab, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    TCITEM tie;
    tie.mask = TCIF_TEXT;
    tie.pszText = (LPWSTR)L"Download status";
    TabCtrl_InsertItem(m_hTab, 0, &tie);
    tie.pszText = (LPWSTR)L"Speed Limiter";
    TabCtrl_InsertItem(m_hTab, 1, &tie);
    tie.pszText = (LPWSTR)L"Options on completion";
    TabCtrl_InsertItem(m_hTab, 2, &tie);

    // 2. White Card Container for Details (Custom Painted or static labels)
    int topY = 44;
    m_hUrlText = CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_PATHELLIPSIS | SS_NOPREFIX,
        24, topY + 8, POPUP_WIDTH - 66, 18,
        m_hWnd, (HMENU)IDC_POPUP_URL_LABEL, m_hInstance, NULL);
    SendMessage(m_hUrlText, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    m_statusCtrls.push_back(m_hUrlText);

    // Left Field Labels
    const wchar_t* labels[] = {
        L"Status", L"File size", L"Downloaded", L"Transfer rate", L"Time left", L"Resume capability"
    };
    int labelY = topY + 30;
    for (int i = 0; i < 6; i++) {
        HWND hLbl = CreateWindowExW(0, L"STATIC", labels[i],
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            24, labelY + (i * 20), 120, 18,
            m_hWnd, NULL, m_hInstance, NULL);
        SendMessage(hLbl, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
        m_statusCtrls.push_back(hLbl);
    }

    // Right Field Values
    m_hStatusVal = CreateWindowExW(0, L"STATIC", L"Connecting...",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        155, labelY + 0, 360, 18,
        m_hWnd, (HMENU)IDC_POPUP_STATUS_LBL, m_hInstance, NULL);
    SendMessage(m_hStatusVal, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);
    m_statusCtrls.push_back(m_hStatusVal);

    m_hSizeVal = CreateWindowExW(0, L"STATIC", L"Unknown",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        155, labelY + 20, 360, 18,
        m_hWnd, (HMENU)IDC_POPUP_SIZE_LBL, m_hInstance, NULL);
    SendMessage(m_hSizeVal, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    m_statusCtrls.push_back(m_hSizeVal);

    m_hDownloadedVal = CreateWindowExW(0, L"STATIC", L"0 Bytes (0.00 %)",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        155, labelY + 40, 360, 18,
        m_hWnd, (HMENU)IDC_POPUP_DOWNLOADED_LBL, m_hInstance, NULL);
    SendMessage(m_hDownloadedVal, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    m_statusCtrls.push_back(m_hDownloadedVal);

    m_hSpeedVal = CreateWindowExW(0, L"STATIC", L"0 KB/sec",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        155, labelY + 60, 360, 18,
        m_hWnd, (HMENU)IDC_POPUP_SPEED_LBL, m_hInstance, NULL);
    SendMessage(m_hSpeedVal, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    m_statusCtrls.push_back(m_hSpeedVal);

    m_hTimeLeftVal = CreateWindowExW(0, L"STATIC", L"--",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        155, labelY + 80, 360, 18,
        m_hWnd, (HMENU)IDC_POPUP_TIMELEFT_LBL, m_hInstance, NULL);
    SendMessage(m_hTimeLeftVal, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    m_statusCtrls.push_back(m_hTimeLeftVal);

    m_hResumeVal = CreateWindowExW(0, L"STATIC", L"Yes",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        155, labelY + 100, 360, 18,
        m_hWnd, (HMENU)IDC_POPUP_RESUME_LBL, m_hInstance, NULL);
    SendMessage(m_hResumeVal, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    m_statusCtrls.push_back(m_hResumeVal);

    // Speed Limiter Controls (Tab 1) - Initially hidden
    m_hChkSpeedLimit = CreateWindowExW(0, L"BUTTON", L"Use Speed Limiter",
        WS_CHILD | BS_AUTOCHECKBOX,
        30, 60, 220, 24,
        m_hWnd, (HMENU)IDC_POPUP_CHK_SPEEDLIMIT, m_hInstance, NULL);
    SendMessage(m_hChkSpeedLimit, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hLblMaxSpeed = CreateWindowExW(0, L"STATIC", L"Maximum download speed:",
        WS_CHILD | SS_LEFT,
        50, 95, 170, 20,
        m_hWnd, NULL, m_hInstance, NULL);
    SendMessage(m_hLblMaxSpeed, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    wchar_t speedStr[32];
    swprintf_s(speedStr, L"%u", m_item ? m_item->speedLimitKBps : 500);
    m_hEditSpeedLimit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", speedStr,
        WS_CHILD | ES_LEFT | ES_NUMBER | ES_AUTOHSCROLL,
        225, 92, 80, 22,
        m_hWnd, (HMENU)IDC_POPUP_EDIT_SPEEDLIMIT, m_hInstance, NULL);
    SendMessage(m_hEditSpeedLimit, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    m_hLblSpeedUnit = CreateWindowExW(0, L"STATIC", L"KBytes/sec",
        WS_CHILD | SS_LEFT,
        315, 95, 100, 20,
        m_hWnd, NULL, m_hInstance, NULL);
    SendMessage(m_hLblSpeedUnit, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    m_hBtnApplySpeed = CreateWindowExW(0, L"BUTTON", L"Apply",
        WS_CHILD | BS_PUSHBUTTON,
        225, 125, 80, 24,
        m_hWnd, (HMENU)IDC_POPUP_BTN_APPLY_SPEED, m_hInstance, NULL);
    SendMessage(m_hBtnApplySpeed, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    m_hLblSpeedStatus = CreateWindowExW(0, L"STATIC", L"Speed limiter is currently: OFF",
        WS_CHILD | SS_LEFT,
        50, 160, 350, 20,
        m_hWnd, NULL, m_hInstance, NULL);
    SendMessage(m_hLblSpeedStatus, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    if (m_item && m_item->speedLimitEnabled) {
        SendMessage(m_hChkSpeedLimit, BM_SETCHECK, BST_CHECKED, 0);
        wchar_t st[128];
        swprintf_s(st, L"Speed limiter is currently: ON (%u KB/sec)", m_item->speedLimitKBps);
        SetWindowTextW(m_hLblSpeedStatus, st);
    } else {
        SendMessage(m_hChkSpeedLimit, BM_SETCHECK, BST_UNCHECKED, 0);
    }

    // 3. Green Progress Bar
    int progY = labelY + 128;
    m_hProgressBar = CreateWindowExW(0, PROGRESS_CLASS, L"",
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        10, progY, POPUP_WIDTH - 36, 16,
        m_hWnd, (HMENU)IDC_POPUP_PROGRESS, m_hInstance, NULL);
    SendMessage(m_hProgressBar, PBM_SETRANGE32, 0, 1000);

    // 4. Action Buttons
    int btnY = progY + 26;
    m_hBtnDetails = CreateWindowExW(0, L"BUTTON", L"<< Hide details",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        18, btnY, 110, 24,
        m_hWnd, (HMENU)IDC_POPUP_BTN_DETAILS, m_hInstance, NULL);
    SendMessage(m_hBtnDetails, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    m_hBtnPause = CreateWindowExW(0, L"BUTTON", L"Pause",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        310, btnY, 95, 24,
        m_hWnd, (HMENU)IDC_POPUP_BTN_PAUSE, m_hInstance, NULL);
    SendMessage(m_hBtnPause, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    m_hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        415, btnY, 95, 24,
        m_hWnd, (HMENU)IDC_POPUP_BTN_CANCEL, m_hInstance, NULL);
    SendMessage(m_hBtnCancel, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    // 5. Connection Section Header
    int connY = btnY + 34;
    m_hConnHeader = CreateWindowExW(0, L"STATIC", L"Start positions and download progress by connections",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, connY, POPUP_WIDTH - 36, 16,
        m_hWnd, (HMENU)IDC_POPUP_CONN_HEADER, m_hInstance, NULL);
    SendMessage(m_hConnHeader, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    // 6. Custom Connection Segment Bar
    int segY = connY + 20;
    m_hSegmentBar = CreateSegmentBar(m_hWnd, m_hInstance, IDC_POPUP_SEGMENT_BAR,
        10, segY, POPUP_WIDTH - 36, 18);

    // 7. Connection Table (ListView)
    int listY = segY + 26;
    m_hConnList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL,
        10, listY, POPUP_WIDTH - 36, 115,
        m_hWnd, (HMENU)IDC_POPUP_CONN_LIST, m_hInstance, NULL);
    SendMessage(m_hConnList, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    ListView_SetExtendedListViewStyle(m_hConnList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    // Add Columns
    LVCOLUMN lvc = { 0 };
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.cx = 40;
    lvc.pszText = (LPWSTR)L"N.";
    ListView_InsertColumn(m_hConnList, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.cx = 120;
    lvc.pszText = (LPWSTR)L"Downloaded";
    ListView_InsertColumn(m_hConnList, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.cx = 350;
    lvc.pszText = (LPWSTR)L"Info";
    ListView_InsertColumn(m_hConnList, 2, &lvc);
}

void DownloadPopup::SwitchTab(int tabIndex) {
    m_currentTab = tabIndex;
    BOOL showStatus = (tabIndex == 0);
    BOOL showSpeed = (tabIndex == 1);

    for (HWND h : m_statusCtrls) {
        if (h) ShowWindow(h, showStatus ? SW_SHOW : SW_HIDE);
    }

    if (m_hChkSpeedLimit) ShowWindow(m_hChkSpeedLimit, showSpeed ? SW_SHOW : SW_HIDE);
    if (m_hLblMaxSpeed) ShowWindow(m_hLblMaxSpeed, showSpeed ? SW_SHOW : SW_HIDE);
    if (m_hEditSpeedLimit) ShowWindow(m_hEditSpeedLimit, showSpeed ? SW_SHOW : SW_HIDE);
    if (m_hLblSpeedUnit) ShowWindow(m_hLblSpeedUnit, showSpeed ? SW_SHOW : SW_HIDE);
    if (m_hBtnApplySpeed) ShowWindow(m_hBtnApplySpeed, showSpeed ? SW_SHOW : SW_HIDE);
    if (m_hLblSpeedStatus) ShowWindow(m_hLblSpeedStatus, showSpeed ? SW_SHOW : SW_HIDE);

    RECT clientRc;
    GetClientRect(m_hWnd, &clientRc);
    RECT cardRc = { 10, 36, clientRc.right - 10, 196 };
    InvalidateRect(m_hWnd, &cardRc, TRUE);
}

void DownloadPopup::ToggleDetails() {
    m_detailsVisible = !m_detailsVisible;
    if (m_detailsVisible) {
        SetWindowTextW(m_hBtnDetails, L"<< Hide details");
        ShowWindow(m_hConnHeader, SW_SHOW);
        ShowWindow(m_hSegmentBar, SW_SHOW);
        ShowWindow(m_hConnList, SW_SHOW);
        SetWindowPos(m_hWnd, NULL, 0, 0, POPUP_WIDTH, POPUP_HEIGHT_EXPANDED, SWP_NOMOVE | SWP_NOZORDER);
    } else {
        SetWindowTextW(m_hBtnDetails, L"Details >>");
        ShowWindow(m_hConnHeader, SW_HIDE);
        ShowWindow(m_hSegmentBar, SW_HIDE);
        ShowWindow(m_hConnList, SW_HIDE);
        SetWindowPos(m_hWnd, NULL, 0, 0, POPUP_WIDTH, POPUP_HEIGHT_COLLAPSED, SWP_NOMOVE | SWP_NOZORDER);
    }
}

static void SetTextIfChanged(HWND hwnd, const std::wstring& newText) {
    if (!hwnd) return;
    wchar_t buf[512] = { 0 };
    GetWindowTextW(hwnd, buf, 512);
    if (wcscmp(buf, newText.c_str()) != 0) {
        SetWindowTextW(hwnd, newText.c_str());
    }
}

void DownloadPopup::UpdateData() {
    if (!m_item || !m_hWnd) return;

    double pct = m_item->GetPercent();

    // 1. Dynamic Window Title
    wchar_t title[512];
    std::wstring displayFn = m_item->fileName;
    if (displayFn.length() > 25) {
        displayFn = displayFn.substr(0, 22) + L"...";
    }
    swprintf_s(title, L"%.0f%% Tafim Internet Download Manager %s", pct, displayFn.c_str());
    SetTextIfChanged(m_hWnd, title);

    // 2. URL
    SetTextIfChanged(m_hUrlText, m_item->url);

    // 3. Status
    std::wstring stStr = StatusToString(m_item->status, m_item->isMerging);
    if (m_item->isMerging) {
        stStr = L"Merging audio and video (DASH)...";
    }
    SetTextIfChanged(m_hStatusVal, stStr);

    // 4. File Size
    uint64_t totalBytes = m_item->GetCombinedTotalBytes();
    std::wstring sizeStr = (totalBytes > 0) ? FormatFileSize(totalBytes) : L"Unknown";
    SetTextIfChanged(m_hSizeVal, sizeStr);

    // 5. Downloaded with percentage
    uint64_t dlBytes = m_item->GetCombinedDownloadedBytes();
    wchar_t dlStr[128];
    swprintf_s(dlStr, L"%s ( %.2f %% )", FormatFileSize(dlBytes).c_str(), pct);
    SetTextIfChanged(m_hDownloadedVal, dlStr);

    // 6. Transfer Rate
    SetTextIfChanged(m_hSpeedVal, FormatSpeed(m_item->speedBytesPerSec));

    // 7. Time Left
    SetTextIfChanged(m_hTimeLeftVal, FormatTimeLeft(m_item->timeLeftSec));

    // 8. Resume Capable
    SetTextIfChanged(m_hResumeVal, m_item->resumeCapable ? L"Yes" : L"No");

    // 9. Progress Bar
    SendMessage(m_hProgressBar, PBM_SETPOS, (WPARAM)(int)(pct * 10.0), 0);

    // 10. Buttons state
    if (m_item->status == DownloadStatus::Complete) {
        SetWindowTextW(m_hBtnPause, L"Open");
        SetWindowTextW(m_hBtnCancel, L"Close");
    } else if (m_item->status == DownloadStatus::Paused || m_item->status == DownloadStatus::Error) {
        SetWindowTextW(m_hBtnPause, L"Resume");
        SetWindowTextW(m_hBtnCancel, L"Cancel");
    } else {
        SetWindowTextW(m_hBtnPause, L"Pause");
        SetWindowTextW(m_hBtnCancel, L"Cancel");
    }

    // 11. Segment Bar
    SegmentBar_SetData(m_hSegmentBar, m_item->connections, m_item->totalBytes);

    // 12. Connection Table
    for (size_t i = 0; i < m_item->connections.size(); i++) {
        if ((int)i >= ListView_GetItemCount(m_hConnList)) {
            LVITEM lvi = { 0 };
            lvi.mask = LVIF_TEXT;
            lvi.iItem = (int)i;
            wchar_t numStr[16];
            swprintf_s(numStr, L"%d", (int)i + 1);
            lvi.pszText = numStr;
            ListView_InsertItem(m_hConnList, &lvi);
        }
        const auto& conn = m_item->connections[i];
        std::wstring dlKb = FormatFileSize(conn.downloadedBytes);
        ListView_SetItemText(m_hConnList, (int)i, 1, (LPWSTR)dlKb.c_str());
        ListView_SetItemText(m_hConnList, (int)i, 2, (LPWSTR)conn.status.c_str());
    }

    while (ListView_GetItemCount(m_hConnList) > (int)m_item->connections.size()) {
        ListView_DeleteItem(m_hConnList, ListView_GetItemCount(m_hConnList) - 1);
    }
}

void DownloadPopup::OnResize(int width, int height) {
    if (!m_hWnd || width <= 0 || height <= 0) return;

    int pad = 10;
    int clientW = width;

    if (m_hTab) {
        SetWindowPos(m_hTab, NULL, pad, 10, clientW - 20, 26, SWP_NOZORDER);
    }
    if (m_hUrlText) {
        SetWindowPos(m_hUrlText, NULL, 24, 52, clientW - 48, 18, SWP_NOZORDER);
    }

    int valW = clientW - 170;
    if (valW < 100) valW = 100;
    if (m_hStatusVal) SetWindowPos(m_hStatusVal, NULL, 155, 74, valW, 18, SWP_NOZORDER);
    if (m_hSizeVal) SetWindowPos(m_hSizeVal, NULL, 155, 94, valW, 18, SWP_NOZORDER);
    if (m_hDownloadedVal) SetWindowPos(m_hDownloadedVal, NULL, 155, 114, valW, 18, SWP_NOZORDER);
    if (m_hSpeedVal) SetWindowPos(m_hSpeedVal, NULL, 155, 134, valW, 18, SWP_NOZORDER);
    if (m_hTimeLeftVal) SetWindowPos(m_hTimeLeftVal, NULL, 155, 154, valW, 18, SWP_NOZORDER);
    if (m_hResumeVal) SetWindowPos(m_hResumeVal, NULL, 155, 174, valW, 18, SWP_NOZORDER);

    int progY = 202;
    if (m_hProgressBar) {
        SetWindowPos(m_hProgressBar, NULL, pad, progY, clientW - 20, 16, SWP_NOZORDER);
    }

    int btnY = progY + 26;
    if (m_hBtnDetails) {
        SetWindowPos(m_hBtnDetails, NULL, 18, btnY, 110, 24, SWP_NOZORDER);
    }
    if (m_hBtnCancel) {
        SetWindowPos(m_hBtnCancel, NULL, clientW - 110, btnY, 95, 24, SWP_NOZORDER);
    }
    if (m_hBtnPause) {
        SetWindowPos(m_hBtnPause, NULL, clientW - 215, btnY, 95, 24, SWP_NOZORDER);
    }

    int connY = btnY + 34;
    if (m_hConnHeader) {
        SetWindowPos(m_hConnHeader, NULL, pad, connY, clientW - 20, 16, SWP_NOZORDER);
    }
    int segY = connY + 20;
    if (m_hSegmentBar) {
        SetWindowPos(m_hSegmentBar, NULL, pad, segY, clientW - 20, 18, SWP_NOZORDER);
    }
    int listY = segY + 26;
    int listH = height - listY - 12;
    if (listH < 60) listH = 60;
    if (m_hConnList) {
        SetWindowPos(m_hConnList, NULL, pad, listY, clientW - 20, listH, SWP_NOZORDER);
    }

    RECT cardRc = { 10, 36, clientW - 10, 196 };
    InvalidateRect(m_hWnd, &cardRc, FALSE);
}

LRESULT CALLBACK DownloadPopup::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    DownloadPopup* pThis = (DownloadPopup*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
        pThis = (DownloadPopup*)pcs->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hWnd = hWnd;
        pThis->CreateControls();
        SetTimer(hWnd, 1, 200, NULL); // Refresh UI every 200ms
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; // Prevent background erasing flash

    case WM_TIMER: {
        if (wParam == 1 && pThis) {
            pThis->UpdateData();
        }
        return 0;
    }

    case WM_NOTIFY: {
        NMHDR* pnm = (NMHDR*)lParam;
        if (pnm && pnm->idFrom == IDC_POPUP_TAB && pnm->code == TCN_SELCHANGE) {
            if (pThis) {
                int cur = TabCtrl_GetCurSel(pThis->m_hTab);
                pThis->SwitchTab(cur);
            }
            return 0;
        }
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_POPUP_BTN_DETAILS) {
            if (pThis) pThis->ToggleDetails();
        } else if (id == IDC_POPUP_BTN_PAUSE) {
            if (pThis && pThis->m_item) {
                if (pThis->m_item->status == DownloadStatus::Complete) {
                    // Open file
                    wchar_t fullPath[MAX_PATH];
                    PathCombineW(fullPath, pThis->m_item->savePath.c_str(), pThis->m_item->fileName.c_str());
                    ShellExecuteW(hWnd, L"open", fullPath, NULL, NULL, SW_SHOWNORMAL);
                } else if (pThis->m_item->status == DownloadStatus::Paused || pThis->m_item->status == DownloadStatus::Error) {
                    pThis->m_engine->ResumeDownload(pThis->m_item->id);
                } else {
                    pThis->m_engine->PauseDownload(pThis->m_item->id);
                }
                pThis->UpdateData();
            }
        } else if (id == IDC_POPUP_BTN_CANCEL) {
            if (pThis && pThis->m_item) {
                if (pThis->m_item->status != DownloadStatus::Complete) {
                    pThis->m_engine->CancelDownload(pThis->m_item->id);
                }
            }
            DestroyWindow(hWnd);
        } else if (id == IDC_POPUP_CHK_SPEEDLIMIT || id == IDC_POPUP_BTN_APPLY_SPEED) {
            if (pThis && pThis->m_item) {
                BOOL isChecked = (SendMessage(pThis->m_hChkSpeedLimit, BM_GETCHECK, 0, 0) == BST_CHECKED);
                wchar_t speedBuf[32] = { 0 };
                GetWindowTextW(pThis->m_hEditSpeedLimit, speedBuf, 32);
                uint32_t kbps = (uint32_t)_wtoi(speedBuf);
                if (kbps == 0) kbps = 500;

                pThis->m_item->speedLimitEnabled = (isChecked != FALSE);
                pThis->m_item->speedLimitKBps = kbps;

                if (pThis->m_item->speedLimitEnabled) {
                    wchar_t st[128];
                    swprintf_s(st, L"Speed limiter is currently: ON (%u KB/sec)", kbps);
                    SetWindowTextW(pThis->m_hLblSpeedStatus, st);
                } else {
                    SetWindowTextW(pThis->m_hLblSpeedStatus, L"Speed limiter is currently: OFF");
                }
            }
        }
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;

        if (pThis && (hwndStatic == pThis->m_hConnHeader ||
                      hwndStatic == pThis->m_hLblMaxSpeed ||
                      hwndStatic == pThis->m_hLblSpeedUnit ||
                      hwndStatic == pThis->m_hLblSpeedStatus)) {
            SetBkMode(hdcStatic, OPAQUE);
            SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE));
            SetTextColor(hdcStatic, RGB(40, 40, 40));
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }

        // Inside the details card: solid white background to eliminate text overlapping
        SetBkMode(hdcStatic, OPAQUE);
        SetBkColor(hdcStatic, RGB(255, 255, 255));

        if (pThis && hwndStatic == pThis->m_hStatusVal) {
            SetTextColor(hdcStatic, RGB(0, 50, 200)); // IDM Blue status text
        } else if (pThis && hwndStatic == pThis->m_hUrlText) {
            SetTextColor(hdcStatic, RGB(0, 80, 160));
        } else {
            SetTextColor(hdcStatic, RGB(0, 0, 0));
        }
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    case WM_SIZE: {
        if (pThis) {
            pThis->OnResize(LOWORD(lParam), HIWORD(lParam));
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT clientRc;
        GetClientRect(hWnd, &clientRc);

        if (pThis && pThis->m_currentTab == 0) {
            // Draw classic white card background for the download details area (Image 1 style)
            RECT cardRc = { 10, 36, clientRc.right - 10, 196 };
            HBRUSH cardBrush = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(hdc, &cardRc, cardBrush);
            DeleteObject(cardBrush);
            DrawEdge(hdc, &cardRc, EDGE_SUNKEN, BF_RECT);
        } else {
            // Draw clean etched background for other tabs
            RECT cardRc = { 10, 36, clientRc.right - 10, 196 };
            FillRect(hdc, &cardRc, GetSysColorBrush(COLOR_BTNFACE));
            DrawEdge(hdc, &cardRc, EDGE_ETCHED, BF_RECT);
        }

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hWnd, 1);
        if (pThis) {
            if (pThis->m_item && pThis->m_item->hwndProgressPopup == hWnd) {
                pThis->m_item->hwndProgressPopup = NULL;
            }
            pThis->m_hWnd = NULL;
        }
        return 0;

    case WM_NCDESTROY:
        if (pThis) {
            delete pThis;
        }
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
