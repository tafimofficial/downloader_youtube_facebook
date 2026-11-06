#include "MainWindow.h"
#include "Resource.h"
#include <gdiplus.h>

#define WC_MAINWINDOW L"TafimMainWindow"

struct ExtDownloadData {
    std::wstring url;
    std::wstring audioUrl;
    bool isDash = false;
    std::wstring filename;
    std::wstring pageUrl;
    std::wstring userAgent;
    std::wstring cookies;
    int qualityHeight = 0;
};

using namespace Gdiplus;

// Helper to draw clean IDM-style icons into ImageList with magenta transparency mask
static HIMAGELIST CreateIDMToolbarImageList() {
    const int iconSize = 32;
    const int numIcons = 12;

    HIMAGELIST hList = ImageList_Create(iconSize, iconSize, ILC_COLOR32, numIcons, 0);

    for (int i = 0; i < numIcons; i++) {
        Bitmap bmp(iconSize, iconSize, PixelFormat32bppARGB);
        Graphics g(&bmp);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.Clear(Color(0, 0, 0, 0));

        switch (i) {
        case 0: { // Add URL (Globe with folder and plus badge)
            SolidBrush bGlobe(Color(255, 0, 150, 255));
            g.FillEllipse(&bGlobe, 3, 3, 22, 22);
            Pen pGlobe(Color(255, 0, 80, 180), 1.5f);
            g.DrawEllipse(&pGlobe, 3, 3, 22, 22);
            g.DrawLine(&pGlobe, 3, 14, 25, 14);
            g.DrawEllipse(&pGlobe, 8, 3, 12, 22);
            SolidBrush bFold(Color(255, 255, 200, 40));
            g.FillRectangle(&bFold, 13, 13, 16, 14);
            SolidBrush bPlus(Color(255, 40, 200, 40));
            g.FillRectangle(&bPlus, 19, 18, 8, 4);
            g.FillRectangle(&bPlus, 21, 16, 4, 8);
            break;
        }
        case 1: { // Resume (3D glossy green arrow)
            Point pts[3] = { Point(6, 4), Point(6, 28), Point(28, 16) };
            SolidBrush bGreen(Color(255, 30, 190, 60));
            g.FillPolygon(&bGreen, pts, 3);
            Pen pGreen(Color(255, 15, 120, 35), 2.0f);
            g.DrawPolygon(&pGreen, pts, 3);
            Pen pHi(Color(255, 160, 255, 180), 1.5f);
            g.DrawLine(&pHi, 8, 7, 24, 16);
            break;
        }
        case 2: { // Stop (Glossy 3D red square)
            SolidBrush bRed(Color(255, 230, 40, 40));
            g.FillRectangle(&bRed, 5, 5, 22, 22);
            Pen pRed(Color(255, 140, 20, 20), 2.0f);
            g.DrawRectangle(&pRed, 5, 5, 22, 22);
            Pen pHi(Color(255, 255, 160, 160), 1.5f);
            g.DrawLine(&pHi, 7, 7, 25, 7);
            break;
        }
        case 3: { // Stop All (Double red pause bars)
            SolidBrush bRed(Color(255, 230, 40, 40));
            g.FillRectangle(&bRed, 5, 5, 9, 22);
            g.FillRectangle(&bRed, 18, 5, 9, 22);
            Pen pRed(Color(255, 140, 20, 20), 1.5f);
            g.DrawRectangle(&pRed, 5, 5, 9, 22);
            g.DrawRectangle(&pRed, 18, 5, 9, 22);
            break;
        }
        case 4: { // Delete (Trash bin with red X badge)
            SolidBrush bBin(Color(255, 180, 185, 195));
            g.FillRectangle(&bBin, 6, 8, 20, 20);
            Pen pBin(Color(255, 100, 105, 115), 1.5f);
            g.DrawRectangle(&pBin, 6, 8, 20, 20);
            Pen pX(Color(255, 230, 30, 30), 3.0f);
            g.DrawLine(&pX, 8, 8, 24, 24);
            g.DrawLine(&pX, 24, 8, 8, 24);
            break;
        }
        case 5: { // Delete Completed (Trash bin with green checkmark)
            SolidBrush bBin(Color(255, 180, 185, 195));
            g.FillRectangle(&bBin, 6, 8, 20, 20);
            Pen pBin(Color(255, 100, 105, 115), 1.5f);
            g.DrawRectangle(&pBin, 6, 8, 20, 20);
            Pen pCheck(Color(255, 30, 190, 60), 3.0f);
            g.DrawLine(&pCheck, 8, 17, 14, 24);
            g.DrawLine(&pCheck, 14, 24, 25, 10);
            break;
        }
        case 6: { // Options (IDM Gear / dials)
            SolidBrush bGear(Color(255, 160, 165, 175));
            g.FillEllipse(&bGear, 4, 4, 24, 24);
            SolidBrush bDial(Color(255, 230, 50, 180));
            g.FillEllipse(&bDial, 8, 8, 16, 16);
            SolidBrush bHole(Color(255, 70, 75, 85));
            g.FillEllipse(&bHole, 12, 12, 8, 8);
            break;
        }
        case 7: { // Scheduler (Clock with hands)
            SolidBrush bClock(Color(255, 255, 200, 40));
            g.FillEllipse(&bClock, 4, 4, 24, 24);
            Pen pBorder(Color(255, 190, 130, 20), 2.0f);
            g.DrawEllipse(&pBorder, 4, 4, 24, 24);
            Pen pHand(Color(255, 40, 40, 40), 2.0f);
            g.DrawLine(&pHand, 16, 16, 16, 8);
            g.DrawLine(&pHand, 16, 16, 22, 16);
            break;
        }
        case 8: { // Start Queue (Folders with green play)
            SolidBrush bFolder(Color(255, 255, 210, 60));
            g.FillRectangle(&bFolder, 4, 5, 24, 18);
            SolidBrush bGreen(Color(255, 30, 190, 60));
            g.FillRectangle(&bGreen, 2, 17, 28, 9);
            break;
        }
        case 9: { // Stop Queue (Folders with red bar)
            SolidBrush bFolder(Color(255, 255, 210, 60));
            g.FillRectangle(&bFolder, 4, 5, 24, 18);
            SolidBrush bRed(Color(255, 230, 40, 40));
            g.FillRectangle(&bRed, 2, 17, 28, 9);
            break;
        }
        case 10: { // Grabber (Search lens)
            SolidBrush bLens(Color(255, 70, 160, 240));
            g.FillEllipse(&bLens, 4, 4, 18, 18);
            Pen pRim(Color(255, 20, 80, 180), 2.0f);
            g.DrawEllipse(&pRim, 4, 4, 18, 18);
            Pen pHandle(Color(255, 60, 60, 60), 3.5f);
            g.DrawLine(&pHandle, 18, 18, 28, 28);
            break;
        }
        case 11: { // Tell a Friend (Mail envelope)
            SolidBrush bEnv(Color(255, 245, 245, 250));
            g.FillRectangle(&bEnv, 3, 7, 26, 17);
            Pen pBorder(Color(255, 120, 120, 140), 1.5f);
            g.DrawRectangle(&pBorder, 3, 7, 26, 17);
            g.DrawLine(&pBorder, 3, 7, 16, 17);
            g.DrawLine(&pBorder, 29, 7, 16, 17);
            SolidBrush bStamp(Color(255, 255, 60, 60));
            g.FillEllipse(&bStamp, 12, 11, 7, 7);
            break;
        }
        }

        HBITMAP hBmp = NULL;
        bmp.GetHBITMAP(Color(0, 0, 0, 0), &hBmp);
        if (hBmp) {
            ImageList_Add(hList, hBmp, NULL);
            DeleteObject(hBmp);
        }
    }

    return hList;
}

LRESULT CALLBACK MainWindow::ModalDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return 0;
    }
    if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLORBTN) {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }
    auto pFn = (std::function<void(HWND, UINT, WPARAM, LPARAM)>*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (pFn && *pFn) {
        if (msg == WM_COMMAND || msg == WM_CLOSE) {
            (*pFn)(hWnd, msg, wParam, lParam);
            if (msg == WM_CLOSE) return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

MainWindow* MainWindow::Create(HINSTANCE hInstance, const std::wstring& cmdLine) {
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWindow::WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = WC_MAINWINDOW;
    wc.hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    RegisterClassEx(&wc);

    WNDCLASSEX wcd = { 0 };
    wcd.cbSize = sizeof(WNDCLASSEX);
    wcd.style = CS_HREDRAW | CS_VREDRAW;
    wcd.lpfnWndProc = MainWindow::ModalDlgProc;
    wcd.hInstance = hInstance;
    wcd.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcd.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcd.lpszClassName = L"TafimModalDialog";
    RegisterClassEx(&wcd);

    RegisterSegmentBarClass(hInstance);

    MainWindow* p = new MainWindow(hInstance, cmdLine);
    return p;
}

MainWindow::MainWindow(HINSTANCE hInstance, const std::wstring& cmdLine)
    : m_hInstance(hInstance) {

    m_hFont = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    m_engine = std::make_unique<DownloadEngine>();
    m_engine->SetCallback(this);

    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 1000;
    int winH = 600;
    int x = (scrW - winW) / 2;
    int y = (scrH - winH) / 2;

    m_hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        WC_MAINWINDOW,
        L"Tafim Internet Download Manager 2.0",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        x, y, winW, winH,
        NULL,
        NULL,
        hInstance,
        this
    );

    HICON hBigIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    HICON hSmallIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    if (hBigIcon) SendMessageW(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hBigIcon);
    if (hSmallIcon) SendMessageW(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hSmallIcon);

    InitTrayIcon();
    SetAutoStart(true);
    StartHttpServer();

    // Load settings from registry or defaults
    HKEY hRegKey;
    wchar_t regDir[MAX_PATH] = { 0 };
    DWORD regSize = sizeof(regDir);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Tafim", 0, KEY_READ, &hRegKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hRegKey, L"DefaultDownloadDir", NULL, NULL, (LPBYTE)regDir, &regSize) == ERROR_SUCCESS) {
            m_defaultDownloadDir = regDir;
        }
        DWORD dwVal = 0, dwValSize = sizeof(dwVal);
        if (RegQueryValueExW(hRegKey, L"ClipboardMonitor", NULL, NULL, (LPBYTE)&dwVal, &dwValSize) == ERROR_SUCCESS) {
            m_clipboardMonitoring = (dwVal != 0);
        }
        dwValSize = sizeof(dwVal);
        if (RegQueryValueExW(hRegKey, L"SoundOnFinish", NULL, NULL, (LPBYTE)&dwVal, &dwValSize) == ERROR_SUCCESS) {
            m_soundOnFinish = (dwVal != 0);
        }
        dwValSize = sizeof(dwVal);
        if (RegQueryValueExW(hRegKey, L"ShutdownWhenDone", NULL, NULL, (LPBYTE)&dwVal, &dwValSize) == ERROR_SUCCESS) {
            m_shutdownWhenDone = (dwVal != 0);
        }
        RegCloseKey(hRegKey);
    }

    if (m_defaultDownloadDir.empty()) {
        wchar_t downFolder[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, downFolder))) {
            PathCombineW(downFolder, downFolder, L"Downloads");
            m_defaultDownloadDir = downFolder;
        } else {
            m_defaultDownloadDir = L"C:\\Downloads";
        }
    }

    if (cmdLine.find(L"/background") != std::wstring::npos || cmdLine.find(L"-background") != std::wstring::npos) {
        m_startMinimized = true;
    }

    if (!cmdLine.empty()) {
        std::wstring url = cmdLine;
        while (!url.empty() && (url.front() == L' ' || url.front() == L'"')) url.erase(url.begin());
        while (!url.empty() && (url.back() == L' ' || url.back() == L'"')) url.pop_back();

        size_t dPos = url.find(L"/d ");
        if (dPos != std::wstring::npos) {
            url = url.substr(dPos + 3);
            while (!url.empty() && (url.front() == L' ' || url.front() == L'"')) url.erase(url.begin());
            while (!url.empty() && (url.back() == L' ' || url.back() == L'"')) url.pop_back();
        }

        if (url.find(L"http://") == 0 || url.find(L"https://") == 0) {
            OnAddUrlSubmitted(url);
        }
    }
}

MainWindow::~MainWindow() {
    StopHttpServer();
    RemoveTrayIcon();
    if (m_hFont) DeleteObject(m_hFont);
}

int MainWindow::Run() {
    if (!m_startMinimized) {
        ShowWindow(m_hWnd, SW_SHOW);
        UpdateWindow(m_hWnd);
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

void MainWindow::InitTrayIcon() {
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hWnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_APP_TRAY_MSG;
    m_nid.hIcon = (HICON)LoadImageW(m_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    if (!m_nid.hIcon) {
        m_nid.hIcon = LoadIconW(m_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    }
    wcscpy_s(m_nid.szTip, L"Tafim Internet Download Manager");
    Shell_NotifyIconW(NIM_ADD, &m_nid);
}

void MainWindow::RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &m_nid);
}

void MainWindow::ShowTrayBalloon(const std::wstring& title, const std::wstring& msg) {
    m_nid.uFlags |= NIF_INFO;
    wcscpy_s(m_nid.szInfoTitle, title.c_str());
    wcscpy_s(m_nid.szInfo, msg.c_str());
    m_nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void MainWindow::ShowContextMenuTray(POINT pt) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN, L"&Open Tafim");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_ADD_URL, L"&Add new download...");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_PAUSE_ALL, L"&Pause all");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RESUME_ALL, L"&Resume all");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"E&xit");

    SetForegroundWindow(m_hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
    DestroyMenu(hMenu);
}

void MainWindow::SetAutoStart(bool enable) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            std::wstring cmd = L"\"" + std::wstring(exePath) + L"\" /background";
            RegSetValueExW(hKey, L"Tafim", 0, REG_SZ, (const BYTE*)cmd.c_str(), (DWORD)((cmd.length() + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hKey, L"Tafim");
        }
        RegCloseKey(hKey);
    }
}

void MainWindow::StartHttpServer() {
    m_httpRunning = true;
    m_httpThread = std::thread(&MainWindow::HttpServerLoop, this);
}

void MainWindow::StopHttpServer() {
    m_httpRunning = false;
    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }
    if (m_httpThread.joinable()) {
        m_httpThread.join();
    }
}

void MainWindow::HttpServerLoop() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return;

    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(9898);

    if (bind(m_listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR ||
        listen(m_listenSocket, 10) == SOCKET_ERROR) {
        closesocket(m_listenSocket);
        WSACleanup();
        return;
    }

    while (m_httpRunning) {
        SOCKET client = accept(m_listenSocket, NULL, NULL);
        if (client == INVALID_SOCKET) {
            if (!m_httpRunning) break;
            continue;
        }

        // Set 5-second socket receive timeout
        DWORD tv = 5000;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

        std::string req;
        char buffer[4096];
        int contentLength = -1;
        size_t headerEnd = std::string::npos;

        while (true) {
            int bytes = recv(client, buffer, sizeof(buffer), 0);
            if (bytes <= 0) break;
            req.append(buffer, bytes);

            if (headerEnd == std::string::npos) {
                headerEnd = req.find("\r\n\r\n");
                if (headerEnd != std::string::npos) {
                    std::string headers = req.substr(0, headerEnd);
                    size_t clPos = headers.find("Content-Length: ");
                    if (clPos == std::string::npos) clPos = headers.find("content-length: ");
                    if (clPos != std::string::npos) {
                        contentLength = atoi(headers.c_str() + clPos + 16);
                    } else {
                        contentLength = 0;
                    }
                }
            }

            if (headerEnd != std::string::npos) {
                int bodyLen = (int)req.length() - (int)(headerEnd + 4);
                if (contentLength <= 0 || bodyLen >= contentLength) {
                    break;
                }
            }
        }

        if (!req.empty()) {
            if (req.find("OPTIONS") == 0) {
                std::string resp = "HTTP/1.1 200 OK\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                    "Access-Control-Allow-Headers: Content-Type, Accept\r\n"
                    "Access-Control-Max-Age: 86400\r\n"
                    "Content-Length: 0\r\n\r\n";
                send(client, resp.c_str(), (int)resp.length(), 0);
            } else if (req.find("GET /ping") == 0 || req.find("GET /status") == 0) {
                std::string body = "{\"status\":\"ok\",\"app\":\"Tafim\"}";
                std::string resp = "HTTP/1.1 200 OK\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: " + std::to_string(body.length()) + "\r\n\r\n" + body;
                send(client, resp.c_str(), (int)resp.length(), 0);
            } else if (req.find("POST /download") == 0) {
                size_t bodyPos = req.find("\r\n\r\n");
                if (bodyPos != std::string::npos) {
                    std::string jsonBody = req.substr(bodyPos + 4);
                    
                    auto extractJsonVal = [&](const std::string& key) -> std::string {
                        std::string search = "\"" + key + "\"";
                        size_t kPos = jsonBody.find(search);
                        if (kPos == std::string::npos) return "";
                        size_t colon = jsonBody.find(':', kPos + search.length());
                        if (colon == std::string::npos) return "";

                        size_t vStart = colon + 1;
                        while (vStart < jsonBody.length() && (jsonBody[vStart] == ' ' || jsonBody[vStart] == '\t' || jsonBody[vStart] == '\r' || jsonBody[vStart] == '\n')) {
                            vStart++;
                        }
                        if (vStart >= jsonBody.length()) return "";

                        if (jsonBody[vStart] == '"') {
                            size_t q1 = vStart;
                            size_t q2 = q1 + 1;
                            while (q2 < jsonBody.length()) {
                                if (jsonBody[q2] == '"' && jsonBody[q2 - 1] != '\\') {
                                    break;
                                }
                                q2++;
                            }
                            if (q2 >= jsonBody.length()) return "";

                            std::string val = jsonBody.substr(q1 + 1, q2 - q1 - 1);
                            std::string unescaped;
                            unescaped.reserve(val.length());
                            for (size_t i = 0; i < val.length(); i++) {
                                if (val[i] == '\\' && i + 1 < val.length()) {
                                    if (val[i + 1] == '/') { unescaped += '/'; i++; }
                                    else if (val[i + 1] == '"') { unescaped += '"'; i++; }
                                    else if (val[i + 1] == '\\') { unescaped += '\\'; i++; }
                                    else { unescaped += val[i]; }
                                } else {
                                    unescaped += val[i];
                                }
                            }
                            return unescaped;
                        } else {
                            size_t vEnd = vStart;
                            while (vEnd < jsonBody.length() && jsonBody[vEnd] != ',' && jsonBody[vEnd] != '}' && jsonBody[vEnd] != ' ' && jsonBody[vEnd] != '\r' && jsonBody[vEnd] != '\n') {
                                vEnd++;
                            }
                            return jsonBody.substr(vStart, vEnd - vStart);
                        }
                    };

                    std::string urlStr = extractJsonVal("url");
                    std::string audioUrlStr = extractJsonVal("audioUrl");
                    std::string isDashStr = extractJsonVal("isDash");
                    std::string fnStr = extractJsonVal("filename");
                    std::string pageUrlStr = extractJsonVal("pageUrl");
                    std::string uaStr = extractJsonVal("userAgent");
                    std::string cookiesStr = extractJsonVal("cookies");
                    std::string qhStr = extractJsonVal("qualityHeight");
                    int qhVal = !qhStr.empty() ? atoi(qhStr.c_str()) : 0;

                    if (!urlStr.empty()) {
                        auto toWStr = [](const std::string& str) -> std::wstring {
                            if (str.empty()) return L"";
                            int wlen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
                            std::wstring res(wlen, 0);
                            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &res[0], wlen);
                            if (!res.empty() && res.back() == L'\0') res.pop_back();
                            return res;
                        };

                        ExtDownloadData* pData = new ExtDownloadData();
                        pData->url = toWStr(urlStr);
                        pData->audioUrl = toWStr(audioUrlStr);
                        pData->isDash = (isDashStr == "true" || !audioUrlStr.empty());
                        pData->filename = toWStr(fnStr);
                        pData->pageUrl = toWStr(pageUrlStr);
                        pData->userAgent = toWStr(uaStr);
                        pData->cookies = toWStr(cookiesStr);
                        pData->qualityHeight = qhVal;

                        PostMessage(m_hWnd, WM_APP_EXT_DOWNLOAD, (WPARAM)pData, 0);
                    }
                }
                std::string body = "{\"success\":true}";
                std::string resp = "HTTP/1.1 200 OK\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: " + std::to_string(body.length()) + "\r\n\r\n" + body;
                send(client, resp.c_str(), (int)resp.length(), 0);
            } else {
                std::string resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                send(client, resp.c_str(), (int)resp.length(), 0);
            }
        }
        shutdown(client, SD_SEND);
        closesocket(client);
    }

    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }
    WSACleanup();
}

void MainWindow::CreateMenus() {
    HMENU hMenuBar = CreateMenu();

    // 1. Tasks Menu
    HMENU hTasks = CreatePopupMenu();
    AppendMenuW(hTasks, MF_STRING, ID_MENU_TASKS_ADD_URL, L"&Add new download...\tCtrl+N");
    AppendMenuW(hTasks, MF_STRING, ID_MENU_TASKS_BATCH, L"&Batch download...");
    AppendMenuW(hTasks, MF_STRING, ID_MENU_TASKS_GRABBER, L"Run &Site Grabber");
    AppendMenuW(hTasks, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hTasks, MF_STRING, ID_MENU_TASKS_EXIT, L"E&xit\tAlt+F4");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hTasks, L"&Tasks");

    // 2. File Menu
    HMENU hFile = CreatePopupMenu();
    AppendMenuW(hFile, MF_STRING, ID_MENU_FILE_STOP, L"&Stop download");
    AppendMenuW(hFile, MF_STRING, ID_MENU_FILE_RESUME, L"&Resume download");
    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFile, MF_STRING, ID_MENU_CALCULATE_HASH, L"Calculate &Checksum (MD5/SHA256)...");
    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFile, MF_STRING, ID_MENU_FILE_DELETE, L"&Delete\tDel");
    AppendMenuW(hFile, MF_STRING, ID_MENU_FILE_DELETE_COMP, L"Delete &Completed");
    AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFile, MF_STRING, ID_MENU_FILE_OPEN, L"&Open");
    AppendMenuW(hFile, MF_STRING, ID_MENU_FILE_OPEN_FOLDER, L"Open &Folder");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hFile, L"&File");

    // 3. Downloads Menu
    HMENU hDownloads = CreatePopupMenu();
    AppendMenuW(hDownloads, MF_STRING, ID_MENU_DOWNLOADS_PAUSE_ALL, L"&Pause all");
    AppendMenuW(hDownloads, MF_STRING, ID_MENU_DOWNLOADS_RESUME_ALL, L"&Resume all");
    AppendMenuW(hDownloads, MF_STRING, ID_MENU_DOWNLOADS_STOP_ALL, L"&Stop all");
    AppendMenuW(hDownloads, MF_SEPARATOR, 0, NULL);
    HMENU hSpeed = CreatePopupMenu();
    AppendMenuW(hSpeed, MF_STRING, ID_MENU_SPEED_LIMIT_ON, L"&Turn on");
    AppendMenuW(hSpeed, MF_STRING, ID_MENU_SPEED_LIMIT_OFF, L"T&urn off");
    AppendMenuW(hSpeed, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hSpeed, MF_STRING, ID_MENU_SPEED_LIMIT_SETTINGS, L"&Settings...");
    AppendMenuW(hDownloads, MF_POPUP, (UINT_PTR)hSpeed, L"Speed &Limiter");
    AppendMenuW(hDownloads, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hDownloads, MF_STRING | (m_clipboardMonitoring ? MF_CHECKED : MF_UNCHECKED), ID_MENU_CLIPBOARD_MONITOR, L"Monitor &Clipboard URLs");
    AppendMenuW(hDownloads, MF_STRING | (m_shutdownWhenDone ? MF_CHECKED : MF_UNCHECKED), ID_MENU_SHUTDOWN_WHEN_DONE, L"Turn &Off Computer when done");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hDownloads, L"&Downloads");

    // 4. View Menu
    HMENU hView = CreatePopupMenu();
    AppendMenuW(hView, MF_STRING | MF_CHECKED, ID_MENU_VIEW_TOOLBAR, L"&Toolbar");
    AppendMenuW(hView, MF_STRING | MF_CHECKED, ID_MENU_VIEW_STATUSBAR, L"&Status bar");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hView, L"&View");

    // 5. Help Menu
    HMENU hHelp = CreatePopupMenu();
    AppendMenuW(hHelp, MF_STRING, ID_MENU_HELP_CONTENTS, L"&Help contents\tF1");
    AppendMenuW(hHelp, MF_STRING, ID_MENU_HELP_ABOUT, L"&About Tafim...");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hHelp, L"&Help");

    // 6. Registration Menu
    HMENU hReg = CreatePopupMenu();
    AppendMenuW(hReg, MF_STRING, ID_MENU_REGISTRATION, L"&Registration...");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hReg, L"&Registration");

    SetMenu(m_hWnd, hMenuBar);
}

void MainWindow::CreateToolbar() {
    m_hToolbar = CreateWindowExW(
        0, TOOLBARCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_TOP | CCS_NODIVIDER,
        0, 0, 0, 0,
        m_hWnd, (HMENU)IDC_MAIN_TOOLBAR, m_hInstance, NULL
    );

    SendMessage(m_hToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    SendMessage(m_hToolbar, TB_SETBITMAPSIZE, 0, MAKELPARAM(32, 32));
    SendMessage(m_hToolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(68, 54));

    HIMAGELIST hImageList = CreateIDMToolbarImageList();
    SendMessage(m_hToolbar, TB_SETIMAGELIST, 0, (LPARAM)hImageList);

    const wchar_t* btnStrings = 
        L"Add URL\0"
        L"Resume\0"
        L"Stop\0"
        L"Stop All\0"
        L"Delete\0"
        L"Delete Co...\0"
        L"Options\0"
        L"Scheduler\0"
        L"Start Queue\0"
        L"Stop Queue\0"
        L"Grabber\0"
        L"Tell a Friend\0\0";
    int strBase = (int)SendMessage(m_hToolbar, TB_ADDSTRINGW, 0, (LPARAM)btnStrings);

    TBBUTTON tbButtons[] = {
        { 0, ID_TOOLBAR_ADD_URL,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)(strBase + 0) },
        { 1, ID_TOOLBAR_RESUME,      TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)(strBase + 1) },
        { 2, ID_TOOLBAR_STOP,        TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)(strBase + 2) },
        { 3, ID_TOOLBAR_STOP_ALL,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)(strBase + 3) },
        { 4, ID_TOOLBAR_DELETE,      TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)(strBase + 4) },
        { 5, ID_TOOLBAR_DELETE_COMP, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)(strBase + 5) },
        { 0, 0,                      0,               BTNS_SEP,    {0}, 0, 0 },
        { 6, ID_TOOLBAR_OPTIONS,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)(strBase + 6) },
        { 7, ID_TOOLBAR_SCHEDULER,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)(strBase + 7) },
        { 8, ID_TOOLBAR_START_QUEUE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)(strBase + 8) },
        { 9, ID_TOOLBAR_STOP_QUEUE,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)(strBase + 9) },
        { 10, ID_TOOLBAR_GRABBER,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)(strBase + 10) },
        { 11, ID_TOOLBAR_TELL_FRIEND,TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)(strBase + 11) }
    };

    SendMessage(m_hToolbar, TB_ADDBUTTONS, (WPARAM)(sizeof(tbButtons) / sizeof(TBBUTTON)), (LPARAM)&tbButtons);
    SendMessage(m_hToolbar, TB_AUTOSIZE, 0, 0);
}

void MainWindow::CreateTreeAndList() {
    m_hTreeView = CreateWindowExW(
        WS_EX_CLIENTEDGE, WC_TREEVIEW, L"",
        WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        0, 0, m_splitterX, 100,
        m_hWnd, (HMENU)IDC_MAIN_TREEVIEW, m_hInstance, NULL
    );
    SendMessage(m_hTreeView, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    m_hListView = CreateWindowExW(
        WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
        m_splitterX + 4, 0, 100, 100,
        m_hWnd, (HMENU)IDC_MAIN_LISTVIEW, m_hInstance, NULL
    );
    SendMessage(m_hListView, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    ListView_SetExtendedListViewStyle(m_hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    struct ColDef {
        const wchar_t* title;
        int width;
    } cols[] = {
        { L"File Name", 180 },
        { L"Q", 35 },
        { L"Size", 90 },
        { L"Status", 85 },
        { L"Description", 90 },
        { L"Transfer rate", 95 },
        { L"Last Try Date", 125 },
        { L"Date Added", 125 },
        { L"Time left", 75 },
        { L"Save To", 260 }
    };

    LVCOLUMN lvc = { 0 };
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    for (int i = 0; i < sizeof(cols) / sizeof(cols[0]); i++) {
        lvc.iSubItem = i;
        lvc.cx = cols[i].width;
        lvc.pszText = (LPWSTR)cols[i].title;
        ListView_InsertColumn(m_hListView, i, &lvc);
    }
}

void MainWindow::CreateStatusBar() {
    m_hStatusBar = CreateWindowExW(
        0, STATUSCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        m_hWnd, (HMENU)IDC_MAIN_STATUSBAR, m_hInstance, NULL
    );
    int parts[] = { 220, 420, -1 };
    SendMessage(m_hStatusBar, SB_SETPARTS, 3, (LPARAM)parts);
    SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"Ready");
    SendMessage(m_hStatusBar, SB_SETTEXTW, 1, (LPARAM)L"Tafim IDM Active");
    UpdateSpeedLimiterStatusBar();
}

void MainWindow::UpdateSpeedLimiterStatusBar() {
    if (!m_hStatusBar || !m_engine) return;
    if (m_engine->IsGlobalSpeedLimitEnabled()) {
        wchar_t buf[128];
        swprintf_s(buf, L"Speed Limiter: ON (%u KB/s) [Click to set]", m_engine->GetGlobalSpeedLimitKBps());
        SendMessage(m_hStatusBar, SB_SETTEXTW, 2, (LPARAM)buf);
    } else {
        SendMessage(m_hStatusBar, SB_SETTEXTW, 2, (LPARAM)L"Speed Limiter: OFF [Click to set]");
    }
}

void MainWindow::PopulateTreeView() {
    TVINSERTSTRUCT tvis = { 0 };
    tvis.hParent = TVI_ROOT;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_STATE;
    tvis.item.state = TVIS_EXPANDED;
    tvis.item.stateMask = TVIS_EXPANDED;

    tvis.item.pszText = (LPWSTR)L"Categories";
    HTREEITEM hCat = TreeView_InsertItem(m_hTreeView, &tvis);

    tvis.hParent = hCat;
    tvis.item.pszText = (LPWSTR)L"All Downloads";
    HTREEITEM hAll = TreeView_InsertItem(m_hTreeView, &tvis);

    const wchar_t* subs[] = { L"Compressed", L"Documents", L"Music", L"Programs", L"Video" };
    tvis.hParent = hAll;
    for (auto sub : subs) {
        tvis.item.pszText = (LPWSTR)sub;
        TreeView_InsertItem(m_hTreeView, &tvis);
    }

    tvis.hParent = hCat;
    tvis.item.pszText = (LPWSTR)L"Unfinished";
    TreeView_InsertItem(m_hTreeView, &tvis);

    tvis.item.pszText = (LPWSTR)L"Finished";
    TreeView_InsertItem(m_hTreeView, &tvis);

    tvis.item.pszText = (LPWSTR)L"Grabber projects";
    TreeView_InsertItem(m_hTreeView, &tvis);

    tvis.item.pszText = (LPWSTR)L"Queues";
    TreeView_InsertItem(m_hTreeView, &tvis);

    TreeView_Expand(m_hTreeView, hCat, TVE_EXPAND);
    TreeView_Expand(m_hTreeView, hAll, TVE_EXPAND);
    TreeView_SelectItem(m_hTreeView, hAll);
}

void MainWindow::RefreshListView() {
    auto allItems = m_engine->GetAllItems();
    std::vector<std::shared_ptr<DownloadItem>> filtered;
    filtered.reserve(allItems.size());

    for (auto& item : allItems) {
        if (m_currentFilter != L"All Downloads" && m_currentFilter != L"Categories") {
            if (m_currentFilter == L"Finished" && item->status != DownloadStatus::Complete) continue;
            if (m_currentFilter == L"Unfinished" && item->status == DownloadStatus::Complete) continue;
            if (m_currentFilter != L"Finished" && m_currentFilter != L"Unfinished" && item->category != m_currentFilter) continue;
        }
        filtered.push_back(item);
    }

    int currentCount = ListView_GetItemCount(m_hListView);
    bool structureMatches = (currentCount == (int)filtered.size());

    if (structureMatches) {
        for (int r = 0; r < currentCount; r++) {
            LVITEM lvi = { 0 };
            lvi.iItem = r;
            lvi.mask = LVIF_PARAM;
            if (!ListView_GetItem(m_hListView, &lvi) || (int)lvi.lParam != filtered[r]->id) {
                structureMatches = false;
                break;
            }
        }
    }

    if (structureMatches) {
        // Fast in-place update: only modify text that changed so selection is never lost
        for (int row = 0; row < currentCount; row++) {
            const auto& item = filtered[row];

            std::wstring sizeStr = (item->GetCombinedTotalBytes() > 0) ? FormatFileSize(item->GetCombinedTotalBytes()) : L"0 Bytes";
            wchar_t curBuf[128] = { 0 };
            ListView_GetItemText(m_hListView, row, 2, curBuf, 128);
            if (wcscmp(curBuf, sizeStr.c_str()) != 0) {
                ListView_SetItemText(m_hListView, row, 2, (LPWSTR)sizeStr.c_str());
            }

            std::wstring stStr = StatusToString(item->status, item->isMerging);
            if (item->isMerging) {
                stStr = L"Merging audio and video (DASH)...";
            } else if (item->status == DownloadStatus::Downloading) {
                wchar_t buf[64];
                swprintf_s(buf, L"%.1f %%", item->GetPercent());
                stStr = buf;
            }
            ListView_GetItemText(m_hListView, row, 3, curBuf, 128);
            if (wcscmp(curBuf, stStr.c_str()) != 0) {
                ListView_SetItemText(m_hListView, row, 3, (LPWSTR)stStr.c_str());
            }

            std::wstring rateStr = (item->status == DownloadStatus::Downloading) ? FormatSpeed(item->speedBytesPerSec) : L"";
            ListView_GetItemText(m_hListView, row, 5, curBuf, 128);
            if (wcscmp(curBuf, rateStr.c_str()) != 0) {
                ListView_SetItemText(m_hListView, row, 5, (LPWSTR)rateStr.c_str());
            }

            std::wstring tlStr = (item->status == DownloadStatus::Downloading) ? FormatTimeLeft(item->timeLeftSec) : L"";
            ListView_GetItemText(m_hListView, row, 8, curBuf, 128);
            if (wcscmp(curBuf, tlStr.c_str()) != 0) {
                ListView_SetItemText(m_hListView, row, 8, (LPWSTR)tlStr.c_str());
            }
        }
        return;
    }

    // Structure changed: save selection, rebuild, and restore selection
    std::vector<int> selIds;
    int sel = -1;
    while ((sel = ListView_GetNextItem(m_hListView, sel, LVNI_SELECTED)) != -1) {
        LVITEM lvi = { 0 };
        lvi.iItem = sel;
        lvi.mask = LVIF_PARAM;
        if (ListView_GetItem(m_hListView, &lvi)) {
            selIds.push_back((int)lvi.lParam);
        }
    }

    SendMessage(m_hListView, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(m_hListView);

    for (int row = 0; row < (int)filtered.size(); row++) {
        const auto& item = filtered[row];

        LVITEM lvi = { 0 };
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = row;
        lvi.iSubItem = 0;
        lvi.pszText = (LPWSTR)item->fileName.c_str();
        lvi.lParam = (LPARAM)item->id;
        ListView_InsertItem(m_hListView, &lvi);

        ListView_SetItemText(m_hListView, row, 1, (LPWSTR)L"");

        std::wstring sizeStr = (item->totalBytes > 0) ? FormatFileSize(item->totalBytes) : L"0 Bytes";
        ListView_SetItemText(m_hListView, row, 2, (LPWSTR)sizeStr.c_str());

        std::wstring stStr = StatusToString(item->status);
        if (item->status == DownloadStatus::Downloading) {
            wchar_t buf[64];
            swprintf_s(buf, L"%.1f %%", item->GetPercent());
            stStr = buf;
        }
        ListView_SetItemText(m_hListView, row, 3, (LPWSTR)stStr.c_str());

        ListView_SetItemText(m_hListView, row, 4, (LPWSTR)item->description.c_str());

        std::wstring rateStr = (item->status == DownloadStatus::Downloading) ? FormatSpeed(item->speedBytesPerSec) : L"";
        ListView_SetItemText(m_hListView, row, 5, (LPWSTR)rateStr.c_str());

        ListView_SetItemText(m_hListView, row, 6, (LPWSTR)item->lastTryDate.c_str());
        ListView_SetItemText(m_hListView, row, 7, (LPWSTR)item->dateAdded.c_str());

        std::wstring tlStr = (item->status == DownloadStatus::Downloading) ? FormatTimeLeft(item->timeLeftSec) : L"";
        ListView_SetItemText(m_hListView, row, 8, (LPWSTR)tlStr.c_str());

        wchar_t fullPath[MAX_PATH];
        PathCombineW(fullPath, item->savePath.c_str(), item->fileName.c_str());
        ListView_SetItemText(m_hListView, row, 9, (LPWSTR)fullPath);

        for (int sid : selIds) {
            if (item->id == sid) {
                ListView_SetItemState(m_hListView, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                break;
            }
        }
    }

    SendMessage(m_hListView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hListView, NULL, FALSE);
}

void MainWindow::OnResize(int width, int height) {
    if (m_hToolbar) SendMessage(m_hToolbar, TB_AUTOSIZE, 0, 0);
    if (m_hStatusBar) SendMessage(m_hStatusBar, WM_SIZE, 0, 0);

    RECT rcTb = { 0 };
    if (m_hToolbar) GetWindowRect(m_hToolbar, &rcTb);
    int tbH = rcTb.bottom - rcTb.top;
    if (tbH == 0) tbH = 60;

    RECT rcSb = { 0 };
    if (m_hStatusBar) GetWindowRect(m_hStatusBar, &rcSb);
    int sbH = rcSb.bottom - rcSb.top;
    if (sbH == 0) sbH = 22;

    int contentY = tbH;
    int contentH = height - tbH - sbH;
    if (contentH < 50) contentH = 50;

    if (m_hTreeView) {
        SetWindowPos(m_hTreeView, NULL, 0, contentY, m_splitterX, contentH, SWP_NOZORDER);
    }
    if (m_hListView) {
        int listX = m_splitterX + 4;
        int listW = width - listX;
        if (listW < 50) listW = 50;
        SetWindowPos(m_hListView, NULL, listX, contentY, listW, contentH, SWP_NOZORDER);
    }
}

void MainWindow::OpenDownloadPopup(std::shared_ptr<DownloadItem> item) {
    if (!item) return;

    if (item->hwndProgressPopup && IsWindow(item->hwndProgressPopup)) {
        ShowWindow(item->hwndProgressPopup, SW_RESTORE);
        SetForegroundWindow(item->hwndProgressPopup);
        return;
    }

    DownloadPopup* popup = DownloadPopup::Create(m_hInstance, NULL, item, m_engine.get());
    m_popups.push_back(popup);
    popup->Show();
}

void MainWindow::OnProgress(int downloadId) {
    PostMessage(m_hWnd, WM_APP_DOWNLOAD_PROGRESS, (WPARAM)downloadId, 0);
}

void MainWindow::OnStatusChange(int downloadId, DownloadStatus status) {
    PostMessage(m_hWnd, WM_APP_DOWNLOAD_STATUS, (WPARAM)downloadId, (LPARAM)status);
}

void MainWindow::OnFinished(int downloadId, bool success, const std::wstring& errorMsg) {
    PostMessage(m_hWnd, WM_APP_DOWNLOAD_FINISHED, (WPARAM)downloadId, (LPARAM)success);
    if (success && m_soundOnFinish) {
        MessageBeep(MB_ICONASTERISK);
    }
    if (m_shutdownWhenDone) {
        CheckShutdownCondition();
    }
}

static void EnableShutdownPrivilege() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
        CloseHandle(hToken);
    }
}

void MainWindow::CheckShutdownCondition() {
    if (!m_shutdownWhenDone || !m_engine) return;
    auto items = m_engine->GetAllItems();
    bool anyActive = false;
    for (const auto& it : items) {
        if (it->status == DownloadStatus::Downloading || it->status == DownloadStatus::Connecting) {
            anyActive = true;
            break;
        }
    }
    if (!anyActive && !items.empty()) {
        int res = MessageBoxW(m_hWnd,
            L"All active downloads have completed!\n\nYour computer will shut down in 15 seconds as configured.\nClick Cancel to abort shutdown.",
            L"Tafim IDM - Automatic Shutdown",
            MB_OKCANCEL | MB_ICONWARNING | MB_DEFBUTTON1);
        if (res == IDOK) {
            EnableShutdownPrivilege();
            InitiateSystemShutdownW(NULL, (LPWSTR)L"Tafim IDM: All downloads complete.", 5, TRUE, FALSE);
        }
    }
}

void MainWindow::ShowChecksumDialog(HWND hParent, const std::wstring& filePath, const std::wstring& fileName) {
    std::wstring md5, sha256;
    uint64_t fileSize = 0;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExW(filePath.c_str(), GetFileExInfoStandard, &fad)) {
        fileSize = ((uint64_t)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
    }

    if (!CalculateFileChecksums(filePath, md5, sha256)) {
        MessageBoxW(hParent ? hParent : m_hWnd, L"Could not open or calculate checksums for this file.\nMake sure the file exists on disk.", L"Checksum Error", MB_OK | MB_ICONERROR);
        return;
    }

    std::wstring dlgTitle = L"Checksum Calculator - " + fileName;
    bool done = false;
    HWND hEditExp = NULL;
    HWND hLblMatch = NULL;

    auto copyToClipboard = [](HWND hOwner, const std::wstring& str) {
        if (OpenClipboard(hOwner)) {
            EmptyClipboard();
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (str.length() + 1) * sizeof(wchar_t));
            if (hMem) {
                wchar_t* pMem = (wchar_t*)GlobalLock(hMem);
                if (pMem) {
                    wcscpy_s(pMem, str.length() + 1, str.c_str());
                    GlobalUnlock(hMem);
                    SetClipboardData(CF_UNICODETEXT, hMem);
                }
            }
            CloseClipboard();
        }
    };

    std::function<void(HWND, UINT, WPARAM, LPARAM)> dlgHandler = [&](HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_COMMAND) {
            int cmd = LOWORD(wParam);
            int code = HIWORD(wParam);
            if (cmd == 3002) {
                copyToClipboard(hDlg, md5);
            } else if (cmd == 3004) {
                copyToClipboard(hDlg, sha256);
            } else if (cmd == 3005 && code == EN_CHANGE) {
                if (hEditExp && hLblMatch) {
                    wchar_t inBuf[256] = { 0 };
                    GetWindowTextW(hEditExp, inBuf, 256);
                    std::wstring inHash = inBuf;
                    while (!inHash.empty() && (inHash.front() == L' ' || inHash.front() == L'"')) inHash.erase(inHash.begin());
                    while (!inHash.empty() && (inHash.back() == L' ' || inHash.back() == L'"')) inHash.pop_back();
                    std::transform(inHash.begin(), inHash.end(), inHash.begin(), ::towlower);

                    if (inHash.empty()) {
                        SetWindowTextW(hLblMatch, L"Enter expected hash above to verify integrity.");
                    } else if (inHash == md5) {
                        SetWindowTextW(hLblMatch, L"[MATCH] MD5 Checksum matches perfectly! (Integrity Verified)");
                    } else if (inHash == sha256) {
                        SetWindowTextW(hLblMatch, L"[MATCH] SHA-256 Checksum matches perfectly! (Integrity Verified)");
                    } else {
                        SetWindowTextW(hLblMatch, L"[DOES NOT MATCH] The hash does not match the file!");
                    }
                }
            } else if (cmd == IDOK || cmd == IDCANCEL) {
                done = true;
                PostMessage(hDlg, WM_NULL, 0, 0);
            }
        } else if (msg == WM_CLOSE) {
            done = true;
            PostMessage(hDlg, WM_NULL, 0, 0);
        }
    };

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"TafimModalDialog", dlgTitle.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        0, 0, 560, 340,
        hParent ? hParent : m_hWnd, NULL, m_hInstance, &dlgHandler
    );

    RECT rcOwner, rcDlg;
    GetWindowRect(hParent ? hParent : m_hWnd, &rcOwner);
    GetWindowRect(hDlg, &rcDlg);
    int x = rcOwner.left + (rcOwner.right - rcOwner.left - 560) / 2;
    int y = rcOwner.top + (rcOwner.bottom - rcOwner.top - 340) / 2;
    SetWindowPos(hDlg, NULL, x, y, 560, 340, SWP_NOZORDER);

    std::wstring fileInfo = L"File: " + fileName + L" (" + FormatFileSize(fileSize) + L")";
    HWND hLblInfo = CreateWindowExW(0, L"STATIC", fileInfo.c_str(), WS_CHILD | WS_VISIBLE | SS_NOPREFIX, 20, 15, 500, 20, hDlg, NULL, m_hInstance, NULL);
    SendMessage(hLblInfo, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // MD5 Row
    HWND hLblMD5 = CreateWindowExW(0, L"STATIC", L"MD5 Checksum:", WS_CHILD | WS_VISIBLE, 20, 48, 110, 20, hDlg, NULL, m_hInstance, NULL);
    SendMessage(hLblMD5, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    HWND hEditMD5 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", md5.c_str(), WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL, 130, 45, 300, 24, hDlg, (HMENU)3001, m_hInstance, NULL);
    SendMessage(hEditMD5, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    HWND hBtnCopyMD5 = CreateWindowExW(0, L"BUTTON", L"Copy MD5", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 440, 45, 90, 24, hDlg, (HMENU)3002, m_hInstance, NULL);
    SendMessage(hBtnCopyMD5, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // SHA256 Row
    HWND hLblSHA = CreateWindowExW(0, L"STATIC", L"SHA-256:", WS_CHILD | WS_VISIBLE, 20, 88, 110, 20, hDlg, NULL, m_hInstance, NULL);
    SendMessage(hLblSHA, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    HWND hEditSHA = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", sha256.c_str(), WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL, 130, 85, 300, 24, hDlg, (HMENU)3003, m_hInstance, NULL);
    SendMessage(hEditSHA, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    HWND hBtnCopySHA = CreateWindowExW(0, L"BUTTON", L"Copy SHA-256", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 440, 85, 90, 24, hDlg, (HMENU)3004, m_hInstance, NULL);
    SendMessage(hBtnCopySHA, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // Verify GroupBox
    HWND hGrp = CreateWindowExW(0, L"BUTTON", L"Hash Verification", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 20, 125, 510, 120, hDlg, NULL, m_hInstance, NULL);
    SendMessage(hGrp, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    HWND hLblExp = CreateWindowExW(0, L"STATIC", L"Paste Expected Hash:", WS_CHILD | WS_VISIBLE, 35, 155, 140, 20, hDlg, NULL, m_hInstance, NULL);
    SendMessage(hLblExp, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    hEditExp = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 180, 152, 335, 24, hDlg, (HMENU)3005, m_hInstance, NULL);
    SendMessage(hEditExp, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    hLblMatch = CreateWindowExW(0, L"STATIC", L"Enter expected hash above to verify integrity.", WS_CHILD | WS_VISIBLE | SS_CENTER, 35, 195, 480, 30, hDlg, (HMENU)3006, m_hInstance, NULL);
    SendMessage(hLblMatch, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    HWND hBtnClose = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 440, 260, 90, 26, hDlg, (HMENU)IDOK, m_hInstance, NULL);
    SendMessage(hBtnClose, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    EnableWindow(hParent ? hParent : m_hWnd, FALSE);

    MSG msg;
    while (!done && GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            done = true;
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    EnableWindow(hParent ? hParent : m_hWnd, TRUE);
    DestroyWindow(hDlg);
    SetForegroundWindow(hParent ? hParent : m_hWnd);
}

static bool PickFolder(HWND hOwner, const std::wstring& currentPath, std::wstring& outPath) {
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = hOwner;
    bi.lpszTitle = L"Select Download Folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path)) {
            outPath = path;
            CoTaskMemFree(pidl);
            return true;
        }
        CoTaskMemFree(pidl);
    }
    return false;
}

void MainWindow::ShowOptionsDialog() {
    bool done = false;
    HWND hEditDir = NULL;
    HWND hCmbConn = NULL;
    HWND hChkClip = NULL;
    HWND hChkSound = NULL;
    HWND hChkShut = NULL;
    HWND hChkStartup = NULL;

    const wchar_t* connOpts[] = { L"1", L"2", L"4", L"8", L"16", L"24", L"32" };

    std::function<void(HWND, UINT, WPARAM, LPARAM)> dlgHandler = [&](HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_COMMAND) {
            int cmd = LOWORD(wParam);
            if (cmd == 1002) {
                std::wstring picked;
                if (PickFolder(hDlg, m_defaultDownloadDir, picked)) {
                    if (hEditDir) SetWindowTextW(hEditDir, picked.c_str());
                }
            } else if (cmd == IDOK) {
                if (hEditDir) {
                    wchar_t buf[MAX_PATH] = { 0 };
                    GetWindowTextW(hEditDir, buf, MAX_PATH);
                    if (wcslen(buf) > 0) {
                        m_defaultDownloadDir = buf;
                    }
                }
                int curSel = hCmbConn ? (int)SendMessage(hCmbConn, CB_GETCURSEL, 0, 0) : -1;
                int conns = 8;
                if (curSel >= 0 && curSel < 7) conns = _wtoi(connOpts[curSel]);
                if (m_engine) m_engine->SetDefaultNumConnections(conns);

                m_clipboardMonitoring = hChkClip ? (SendMessage(hChkClip, BM_GETCHECK, 0, 0) == BST_CHECKED) : true;
                m_soundOnFinish = hChkSound ? (SendMessage(hChkSound, BM_GETCHECK, 0, 0) == BST_CHECKED) : true;
                m_shutdownWhenDone = hChkShut ? (SendMessage(hChkShut, BM_GETCHECK, 0, 0) == BST_CHECKED) : false;
                bool autoStart = hChkStartup ? (SendMessage(hChkStartup, BM_GETCHECK, 0, 0) == BST_CHECKED) : true;
                SetAutoStart(autoStart);

                HKEY hKey;
                if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Tafim", 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                    RegSetValueExW(hKey, L"DefaultDownloadDir", 0, REG_SZ, (const BYTE*)m_defaultDownloadDir.c_str(), (DWORD)((m_defaultDownloadDir.length() + 1) * sizeof(wchar_t)));
                    DWORD dwConns = (DWORD)conns;
                    RegSetValueExW(hKey, L"MaxConnections", 0, REG_DWORD, (const BYTE*)&dwConns, sizeof(dwConns));
                    DWORD dwClip = m_clipboardMonitoring ? 1 : 0;
                    RegSetValueExW(hKey, L"ClipboardMonitor", 0, REG_DWORD, (const BYTE*)&dwClip, sizeof(dwClip));
                    DWORD dwSound = m_soundOnFinish ? 1 : 0;
                    RegSetValueExW(hKey, L"SoundOnFinish", 0, REG_DWORD, (const BYTE*)&dwSound, sizeof(dwSound));
                    DWORD dwShut = m_shutdownWhenDone ? 1 : 0;
                    RegSetValueExW(hKey, L"ShutdownWhenDone", 0, REG_DWORD, (const BYTE*)&dwShut, sizeof(dwShut));
                    RegCloseKey(hKey);
                }
                done = true;
                PostMessage(hDlg, WM_NULL, 0, 0);
            } else if (cmd == IDCANCEL) {
                done = true;
                PostMessage(hDlg, WM_NULL, 0, 0);
            }
        } else if (msg == WM_CLOSE) {
            done = true;
            PostMessage(hDlg, WM_NULL, 0, 0);
        }
    };

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"TafimModalDialog", L"Tafim IDM - Options & Preferences",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        0, 0, 530, 360,
        m_hWnd, NULL, m_hInstance, &dlgHandler
    );

    RECT rcOwner, rcDlg;
    GetWindowRect(m_hWnd, &rcOwner);
    GetWindowRect(hDlg, &rcDlg);
    int x = rcOwner.left + (rcOwner.right - rcOwner.left - 530) / 2;
    int y = rcOwner.top + (rcOwner.bottom - rcOwner.top - 360) / 2;
    SetWindowPos(hDlg, NULL, x, y, 530, 360, SWP_NOZORDER);

    // 1. General Box
    HWND hGrpGeneral = CreateWindowExW(0, L"BUTTON", L"General && Downloads", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 12, 490, 115, hDlg, NULL, m_hInstance, NULL);
    SendMessage(hGrpGeneral, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    HWND hLblDir = CreateWindowExW(0, L"STATIC", L"Default Download Folder:", WS_CHILD | WS_VISIBLE, 30, 35, 180, 18, hDlg, NULL, m_hInstance, NULL);
    SendMessage(hLblDir, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    hEditDir = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", m_defaultDownloadDir.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 30, 56, 360, 24, hDlg, (HMENU)1001, m_hInstance, NULL);
    SendMessage(hEditDir, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    HWND hBtnBrowse = CreateWindowExW(0, L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 400, 56, 85, 24, hDlg, (HMENU)1002, m_hInstance, NULL);
    SendMessage(hBtnBrowse, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    HWND hLblConn = CreateWindowExW(0, L"STATIC", L"Default Max Connections Number:", WS_CHILD | WS_VISIBLE, 30, 92, 210, 18, hDlg, NULL, m_hInstance, NULL);
    SendMessage(hLblConn, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    hCmbConn = CreateWindowExW(0, WC_COMBOBOX, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 245, 89, 70, 160, hDlg, (HMENU)1003, m_hInstance, NULL);
    SendMessage(hCmbConn, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    int curConns = m_engine ? m_engine->GetDefaultNumConnections() : 8;
    int selIdx = 3; // default 8
    for (int i = 0; i < 7; i++) {
        SendMessageW(hCmbConn, CB_ADDSTRING, 0, (LPARAM)connOpts[i]);
        if (_wtoi(connOpts[i]) == curConns) selIdx = i;
    }
    SendMessage(hCmbConn, CB_SETCURSEL, selIdx, 0);

    // 2. Automation Box
    HWND hGrpAuto = CreateWindowExW(0, L"BUTTON", L"Automation && Sounds", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 138, 490, 135, hDlg, NULL, m_hInstance, NULL);
    SendMessage(hGrpAuto, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    hChkClip = CreateWindowExW(0, L"BUTTON", L"Monitor clipboard for copied downloadable URLs", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 30, 162, 450, 20, hDlg, (HMENU)1004, m_hInstance, NULL);
    SendMessage(hChkClip, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    SendMessage(hChkClip, BM_SETCHECK, m_clipboardMonitoring ? BST_CHECKED : BST_UNCHECKED, 0);

    hChkSound = CreateWindowExW(0, L"BUTTON", L"Play sound effect when download completes", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 30, 188, 450, 20, hDlg, (HMENU)1005, m_hInstance, NULL);
    SendMessage(hChkSound, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    SendMessage(hChkSound, BM_SETCHECK, m_soundOnFinish ? BST_CHECKED : BST_UNCHECKED, 0);

    hChkShut = CreateWindowExW(0, L"BUTTON", L"Turn off / Shutdown computer when all downloads finish", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 30, 214, 450, 20, hDlg, (HMENU)1006, m_hInstance, NULL);
    SendMessage(hChkShut, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    SendMessage(hChkShut, BM_SETCHECK, m_shutdownWhenDone ? BST_CHECKED : BST_UNCHECKED, 0);

    hChkStartup = CreateWindowExW(0, L"BUTTON", L"Launch Tafim automatically on Windows startup", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 30, 240, 450, 20, hDlg, (HMENU)1007, m_hInstance, NULL);
    SendMessage(hChkStartup, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    SendMessage(hChkStartup, BM_SETCHECK, BST_CHECKED, 0);

    HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 305, 285, 95, 26, hDlg, (HMENU)IDOK, m_hInstance, NULL);
    SendMessage(hBtnOk, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 410, 285, 95, 26, hDlg, (HMENU)IDCANCEL, m_hInstance, NULL);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    EnableWindow(m_hWnd, FALSE);
    SetFocus(hEditDir);

    MSG msg;
    while (!done && GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_RETURN) {
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)hBtnOk);
                continue;
            } else if (msg.wParam == VK_ESCAPE) {
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), (LPARAM)hBtnCancel);
                continue;
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    EnableWindow(m_hWnd, TRUE);
    DestroyWindow(hDlg);
    SetForegroundWindow(m_hWnd);
}

void MainWindow::ShowAddUrlDialog() {
    bool dialogDone = false;
    std::wstring enteredUrl;
    std::wstring enteredFolder = m_defaultDownloadDir;
    HWND hEdit = NULL;
    HWND hEditPath = NULL;

    std::function<void(HWND, UINT, WPARAM, LPARAM)> dlgHandler = [&](HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_COMMAND) {
            int cmd = LOWORD(wParam);
            if (cmd == 1004) {
                std::wstring picked;
                if (PickFolder(hDlg, enteredFolder, picked)) {
                    enteredFolder = picked;
                    if (hEditPath) SetWindowTextW(hEditPath, picked.c_str());
                }
            } else if (cmd == IDOK) {
                if (hEdit) {
                    wchar_t buf[2048] = { 0 };
                    GetWindowTextW(hEdit, buf, 2048);
                    enteredUrl = buf;
                }
                if (hEditPath) {
                    wchar_t pBuf[MAX_PATH] = { 0 };
                    GetWindowTextW(hEditPath, pBuf, MAX_PATH);
                    enteredFolder = pBuf;
                }
                dialogDone = true;
                PostMessage(hDlg, WM_NULL, 0, 0);
            } else if (cmd == IDCANCEL) {
                dialogDone = true;
                PostMessage(hDlg, WM_NULL, 0, 0);
            }
        } else if (msg == WM_CLOSE) {
            dialogDone = true;
            PostMessage(hDlg, WM_NULL, 0, 0);
        }
    };

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"TafimModalDialog", L"Enter new address to download",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        0, 0, 530, 210,
        m_hWnd, NULL, m_hInstance, &dlgHandler
    );

    RECT rcOwner, rcDlg;
    GetWindowRect(m_hWnd, &rcOwner);
    GetWindowRect(hDlg, &rcDlg);
    int x = rcOwner.left + (rcOwner.right - rcOwner.left - (rcDlg.right - rcDlg.left)) / 2;
    int y = rcOwner.top + (rcOwner.bottom - rcOwner.top - (rcDlg.bottom - rcDlg.top)) / 2;
    SetWindowPos(hDlg, NULL, x, y, 530, 210, SWP_NOZORDER);

    // 1. Address Row
    HWND hLbl = CreateWindowExW(0, L"STATIC", L"Address:", WS_CHILD | WS_VISIBLE, 15, 20, 65, 20, hDlg, NULL, m_hInstance, NULL);
    SendMessage(hLbl, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        85, 18, 410, 24, hDlg, (HMENU)IDC_ADD_URL_EDIT, m_hInstance, NULL);
    SendMessage(hEdit, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // 2. Save To Row
    HWND hLblSave = CreateWindowExW(0, L"STATIC", L"Save To:", WS_CHILD | WS_VISIBLE, 15, 58, 65, 20, hDlg, NULL, m_hInstance, NULL);
    SendMessage(hLblSave, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    hEditPath = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", m_defaultDownloadDir.c_str(),
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        85, 56, 315, 24, hDlg, (HMENU)1003, m_hInstance, NULL);
    SendMessage(hEditPath, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    HWND hBtnBrowse = CreateWindowExW(0, L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        410, 56, 85, 24, hDlg, (HMENU)1004, m_hInstance, NULL);
    SendMessage(hBtnBrowse, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    if (OpenClipboard(m_hWnd)) {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
            wchar_t* text = (wchar_t*)GlobalLock(hData);
            if (text && (wcsstr(text, L"http://") || wcsstr(text, L"https://"))) {
                SetWindowTextW(hEdit, text);
            }
            GlobalUnlock(hData);
        }
        CloseClipboard();
    }

    HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        305, 115, 90, 26, hDlg, (HMENU)IDOK, m_hInstance, NULL);
    SendMessage(hBtnOk, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        405, 115, 90, 26, hDlg, (HMENU)IDCANCEL, m_hInstance, NULL);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    EnableWindow(m_hWnd, FALSE);
    SetFocus(hEdit);

    MSG msg;
    while (!dialogDone && GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_RETURN) {
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)hBtnOk);
                continue;
            } else if (msg.wParam == VK_ESCAPE) {
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), (LPARAM)hBtnCancel);
                continue;
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    EnableWindow(m_hWnd, TRUE);
    DestroyWindow(hDlg);
    SetForegroundWindow(m_hWnd);

    if (!enteredUrl.empty()) {
        OnAddUrlSubmitted(enteredUrl, L"", enteredFolder);
    }
}

void MainWindow::OnAddUrlSubmitted(const std::wstring& url, const std::wstring& filename, const std::wstring& customFolder,
    const std::wstring& userAgent, const std::wstring& cookies, const std::wstring& referer,
    const std::wstring& audioUrl, bool isDash, int qualityHeight) {

    // Prevent duplicate downloads from running concurrently and consuming extra disk space
    auto allItems = m_engine->GetAllItems();
    for (auto& existing : allItems) {
        if (existing && existing->url == url &&
            (existing->status == DownloadStatus::Downloading || existing->status == DownloadStatus::Connecting)) {
            OpenDownloadPopup(existing);
            return;
        }
    }

    std::wstring downFolder = !customFolder.empty() ? customFolder : m_defaultDownloadDir;
    if (downFolder.empty()) {
        wchar_t defPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, defPath))) {
            PathCombineW(defPath, defPath, L"Downloads");
            downFolder = defPath;
        } else {
            downFolder = L"C:\\Downloads";
        }
    }

    std::wstring cat = (isDash || !audioUrl.empty() || url.find(L"video") != std::wstring::npos) ? L"Video" : L"Programs";
    int id = m_engine->AddDownload(url, downFolder, filename, cat, userAgent, cookies, referer, audioUrl, isDash, qualityHeight);
    auto item = m_engine->GetItem(id);

    RefreshListView();
    m_engine->StartDownload(id);
    OpenDownloadPopup(item);
}

void MainWindow::OnResumeSelected() {
    int sel = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (sel != -1) {
        LVITEM lvi = { 0 };
        lvi.iItem = sel;
        lvi.mask = LVIF_PARAM;
        ListView_GetItem(m_hListView, &lvi);
        int id = (int)lvi.lParam;
        m_engine->ResumeDownload(id);
        auto item = m_engine->GetItem(id);
        OpenDownloadPopup(item);
        RefreshListView();
    }
}

void MainWindow::OnStopSelected() {
    int sel = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (sel != -1) {
        LVITEM lvi = { 0 };
        lvi.iItem = sel;
        lvi.mask = LVIF_PARAM;
        ListView_GetItem(m_hListView, &lvi);
        int id = (int)lvi.lParam;
        m_engine->PauseDownload(id);
        RefreshListView();
    }
}

void MainWindow::OnStopAll() {
    auto items = m_engine->GetAllItems();
    for (auto& it : items) {
        if (it->status == DownloadStatus::Downloading) {
            m_engine->PauseDownload(it->id);
        }
    }
    RefreshListView();
}

void MainWindow::ShowSpeedLimiterDialog() {
    bool done = false;
    HWND hChk = NULL;
    HWND hEdit = NULL;

    std::function<void(HWND, UINT, WPARAM, LPARAM)> dlgHandler = [&](HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_COMMAND) {
            int cmd = LOWORD(wParam);
            if (cmd == IDOK) {
                BOOL enabled = hChk ? (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) : FALSE;
                wchar_t valStr[32] = { 0 };
                if (hEdit) GetWindowTextW(hEdit, valStr, 32);
                uint32_t val = (uint32_t)_wtoi(valStr);
                if (val == 0) val = 500;
                m_engine->SetGlobalSpeedLimit(enabled != FALSE, val);
                UpdateSpeedLimiterStatusBar();
                done = true;
                PostMessage(hDlg, WM_NULL, 0, 0);
            } else if (cmd == IDCANCEL) {
                done = true;
                PostMessage(hDlg, WM_NULL, 0, 0);
            }
        } else if (msg == WM_CLOSE) {
            done = true;
            PostMessage(hDlg, WM_NULL, 0, 0);
        }
    };

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"TafimModalDialog", L"Speed Limiter Settings",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        0, 0, 420, 210,
        m_hWnd, NULL, m_hInstance, &dlgHandler
    );

    RECT rcOwner, rcDlg;
    GetWindowRect(m_hWnd, &rcOwner);
    GetWindowRect(hDlg, &rcDlg);
    int x = rcOwner.left + (rcOwner.right - rcOwner.left - 420) / 2;
    int y = rcOwner.top + (rcOwner.bottom - rcOwner.top - 210) / 2;
    SetWindowPos(hDlg, NULL, x, y, 420, 210, SWP_NOZORDER);

    hChk = CreateWindowExW(0, L"BUTTON", L"Use Speed Limiter",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        25, 20, 200, 24, hDlg, (HMENU)2001, m_hInstance, NULL);
    SendMessage(hChk, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    if (m_engine->IsGlobalSpeedLimitEnabled()) {
        SendMessage(hChk, BM_SETCHECK, BST_CHECKED, 0);
    }

    HWND hLbl = CreateWindowExW(0, L"STATIC", L"Maximum download speed for each process:",
        WS_CHILD | WS_VISIBLE, 25, 58, 260, 20, hDlg, NULL, m_hInstance, NULL);
    SendMessage(hLbl, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    wchar_t speedBuf[32];
    swprintf_s(speedBuf, L"%u", m_engine->GetGlobalSpeedLimitKBps() > 0 ? m_engine->GetGlobalSpeedLimitKBps() : 500);
    hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", speedBuf,
        WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_AUTOHSCROLL,
        290, 55, 60, 22, hDlg, (HMENU)2002, m_hInstance, NULL);
    SendMessage(hEdit, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    HWND hUnit = CreateWindowExW(0, L"STATIC", L"KBytes/sec",
        WS_CHILD | WS_VISIBLE, 355, 58, 50, 20, hDlg, NULL, m_hInstance, NULL);
    SendMessage(hUnit, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"OK",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        200, 120, 85, 26, hDlg, (HMENU)IDOK, m_hInstance, NULL);
    SendMessage(hBtnOk, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        295, 120, 85, 26, hDlg, (HMENU)IDCANCEL, m_hInstance, NULL);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    EnableWindow(m_hWnd, FALSE);
    SetFocus(hEdit);

    MSG msg;
    while (!done && GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_RETURN) {
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)hBtnOk);
                continue;
            } else if (msg.wParam == VK_ESCAPE) {
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), (LPARAM)hBtnCancel);
                continue;
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    EnableWindow(m_hWnd, TRUE);
    DestroyWindow(hDlg);
    SetForegroundWindow(m_hWnd);
}

void MainWindow::OnDeleteSelected() {
    std::vector<int> idsToDelete;
    int sel = -1;
    while ((sel = ListView_GetNextItem(m_hListView, sel, LVNI_SELECTED)) != -1) {
        LVITEM lvi = { 0 };
        lvi.iItem = sel;
        lvi.mask = LVIF_PARAM;
        if (ListView_GetItem(m_hListView, &lvi)) {
            idsToDelete.push_back((int)lvi.lParam);
        }
    }

    if (idsToDelete.empty()) return;

    int res = MessageBoxW(m_hWnd,
        L"Do you also want to delete the file(s) from your disk to free up storage?",
        L"Delete Download",
        MB_YESNOCANCEL | MB_ICONQUESTION);
    if (res == IDCANCEL) return;

    bool deleteFiles = (res == IDYES);
    for (int id : idsToDelete) {
        m_engine->DeleteDownload(id, deleteFiles);
    }
    RefreshListView();
}

void MainWindow::OnDeleteCompleted() {
    auto items = m_engine->GetAllItems();
    for (auto& it : items) {
        if (it->status == DownloadStatus::Complete) {
            m_engine->DeleteDownload(it->id, false);
        }
    }
    RefreshListView();
}

void MainWindow::OnRefreshAddress() {
    int sel = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    if (sel == -1) return;

    LVITEM lvi = { 0 };
    lvi.iItem = sel;
    lvi.mask = LVIF_PARAM;
    if (!ListView_GetItem(m_hListView, &lvi)) return;
    int selectedId = (int)lvi.lParam;

    auto item = m_engine->GetItem(selectedId);
    if (!item) return;

    bool done = false;
    HWND hEdit = NULL;

    std::function<void(HWND, UINT, WPARAM, LPARAM)> dlgHandler = [&](HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_COMMAND) {
            int cmd = LOWORD(wParam);
            if (cmd == IDOK) {
                if (hEdit) {
                    wchar_t newUrlBuf[4096] = { 0 };
                    GetWindowTextW(hEdit, newUrlBuf, 4096);
                    std::wstring newUrl = newUrlBuf;
                    if (!newUrl.empty() && (newUrl.find(L"http://") == 0 || newUrl.find(L"https://") == 0)) {
                        m_engine->UpdateDownloadUrl(selectedId, newUrl);
                        m_engine->ResumeDownload(selectedId);
                        RefreshListView();
                    }
                }
                done = true;
                PostMessage(hDlg, WM_NULL, 0, 0);
            } else if (cmd == IDCANCEL) {
                done = true;
                PostMessage(hDlg, WM_NULL, 0, 0);
            }
        } else if (msg == WM_CLOSE) {
            done = true;
            PostMessage(hDlg, WM_NULL, 0, 0);
        }
    };

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"TafimModalDialog", L"Refresh Download Address",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        0, 0, 520, 180,
        m_hWnd, NULL, m_hInstance, &dlgHandler
    );

    RECT rcOwner, rcDlg;
    GetWindowRect(m_hWnd, &rcOwner);
    GetWindowRect(hDlg, &rcDlg);
    int x = rcOwner.left + (rcOwner.right - rcOwner.left - (rcDlg.right - rcDlg.left)) / 2;
    int y = rcOwner.top + (rcOwner.bottom - rcOwner.top - (rcDlg.bottom - rcDlg.top)) / 2;
    SetWindowPos(hDlg, NULL, x, y, 520, 180, SWP_NOZORDER);

    HWND hLbl = CreateWindowExW(0, L"STATIC", L"Enter new / refreshed address for this download:", WS_CHILD | WS_VISIBLE, 15, 15, 480, 20, hDlg, NULL, m_hInstance, NULL);
    SendMessage(hLbl, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", item->url.c_str(),
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        15, 40, 475, 24, hDlg, (HMENU)1001, m_hInstance, NULL);
    SendMessage(hEdit, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // If clipboard has a fresh URL, paste it
    if (OpenClipboard(m_hWnd)) {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
            wchar_t* text = (wchar_t*)GlobalLock(hData);
            if (text && (wcsstr(text, L"http://") || wcsstr(text, L"https://"))) {
                SetWindowTextW(hEdit, text);
            }
            GlobalUnlock(hData);
        }
        CloseClipboard();
    }

    HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"OK",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        285, 95, 100, 26, hDlg, (HMENU)IDOK, m_hInstance, NULL);
    SendMessage(hBtnOk, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        390, 95, 100, 26, hDlg, (HMENU)IDCANCEL, m_hInstance, NULL);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    EnableWindow(m_hWnd, FALSE);
    SetFocus(hEdit);
    SendMessage(hEdit, EM_SETSEL, 0, -1);

    MSG msg;
    while (!done && GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_RETURN) {
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)hBtnOk);
                continue;
            } else if (msg.wParam == VK_ESCAPE) {
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), (LPARAM)hBtnCancel);
                continue;
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    EnableWindow(m_hWnd, TRUE);
    DestroyWindow(hDlg);
    SetForegroundWindow(m_hWnd);
}

LRESULT CALLBACK MainWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = (MainWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
        pThis = (MainWindow*)pcs->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hWnd = hWnd;

        pThis->CreateMenus();
        pThis->CreateToolbar();
        pThis->CreateTreeAndList();
        pThis->CreateStatusBar();
        pThis->PopulateTreeView();
        pThis->RefreshListView();

        AddClipboardFormatListener(hWnd);
        DragAcceptFiles(hWnd, TRUE);

        SetTimer(hWnd, 10, 500, NULL);
        return 0;
    }

    case WM_CLIPBOARDUPDATE: {
        if (pThis && pThis->m_clipboardMonitoring && OpenClipboard(hWnd)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData) {
                wchar_t* text = (wchar_t*)GlobalLock(hData);
                if (text) {
                    std::wstring url = text;
                    while (!url.empty() && (url.front() == L' ' || url.front() == L'\r' || url.front() == L'\n' || url.front() == L'\t')) url.erase(url.begin());
                    while (!url.empty() && (url.back() == L' ' || url.back() == L'\r' || url.back() == L'\n' || url.back() == L'\t')) url.pop_back();

                    if ((url.find(L"http://") == 0 || url.find(L"https://") == 0) && url != pThis->m_lastClipboardUrl) {
                        pThis->m_lastClipboardUrl = url;
                        // Check if URL looks downloadable
                        std::wstring lower = url;
                        std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
                        bool isDl = (lower.find(L".zip") != std::wstring::npos || lower.find(L".rar") != std::wstring::npos ||
                                     lower.find(L".7z") != std::wstring::npos || lower.find(L".tar") != std::wstring::npos ||
                                     lower.find(L".gz") != std::wstring::npos || lower.find(L".iso") != std::wstring::npos ||
                                     lower.find(L".exe") != std::wstring::npos || lower.find(L".msi") != std::wstring::npos ||
                                     lower.find(L".apk") != std::wstring::npos || lower.find(L".mp4") != std::wstring::npos ||
                                     lower.find(L".mkv") != std::wstring::npos || lower.find(L".mp3") != std::wstring::npos ||
                                     lower.find(L".pdf") != std::wstring::npos || lower.find(L"/releases/download/") != std::wstring::npos ||
                                     lower.find(L"youtube.com/watch") != std::wstring::npos || lower.find(L"youtu.be/") != std::wstring::npos ||
                                     lower.find(L"facebook.com/") != std::wstring::npos || lower.find(L"fb.watch/") != std::wstring::npos ||
                                     lower.find(L"instagram.com/") != std::wstring::npos || lower.find(L"tiktok.com/") != std::wstring::npos);

                        if (isDl) {
                            GlobalUnlock(hData);
                            CloseClipboard();
                            pThis->ShowAddUrlDialog();
                            return 0;
                        }
                    }
                    GlobalUnlock(hData);
                }
            }
            CloseClipboard();
        }
        return 0;
    }

    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wParam;
        wchar_t droppedPath[MAX_PATH] = { 0 };
        if (pThis && DragQueryFileW(hDrop, 0, droppedPath, MAX_PATH) > 0) {
            if (GetFileAttributesW(droppedPath) != INVALID_FILE_ATTRIBUTES && !(GetFileAttributesW(droppedPath) & FILE_ATTRIBUTE_DIRECTORY)) {
                wchar_t* pfn = PathFindFileNameW(droppedPath);
                pThis->ShowChecksumDialog(hWnd, droppedPath, pfn ? pfn : L"File");
            }
        }
        DragFinish(hDrop);
        return 0;
    }

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, GetSysColorBrush(COLOR_BTNFACE));
        return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        FillRect(hdc, &ps.rcPaint, GetSysColorBrush(COLOR_BTNFACE));
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_SIZE: {
        if (pThis) {
            pThis->OnResize(LOWORD(lParam), HIWORD(lParam));
        }
        return 0;
    }

    case WM_TIMER: {
        if (wParam == 10 && pThis) {
            pThis->RefreshListView();
        }
        return 0;
    }

    case WM_APP_DOWNLOAD_PROGRESS: {
        if (pThis) {
            pThis->RefreshListView();
        }
        return 0;
    }

    case WM_APP_DOWNLOAD_STATUS:
    case WM_APP_DOWNLOAD_FINISHED: {
        if (pThis) {
            pThis->RefreshListView();
            if (pThis->m_engine) pThis->m_engine->SaveDownloads();
        }
        return 0;
    }

    case WM_APP_EXT_DOWNLOAD: {
        auto pData = (ExtDownloadData*)wParam;
        if (pData && pThis) {
            // Keep main window in tray/background. Only popup window will show!
            pThis->OnAddUrlSubmitted(pData->url, pData->filename, L"", pData->userAgent, pData->cookies, pData->pageUrl, pData->audioUrl, pData->isDash, pData->qualityHeight);
            delete pData;
        }
        return 0;
    }

    case WM_APP_TRAY_MSG: {
        if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP) {
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
        } else if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            if (pThis) pThis->ShowContextMenuTray(pt);
        }
        return 0;
    }

    case WM_NOTIFY: {
        LPNMHDR pnm = (LPNMHDR)lParam;
        if (pThis && pnm->hwndFrom == pThis->m_hListView) {
            if (pnm->code == LVN_KEYDOWN) {
                LPNMLVKEYDOWN pnkd = (LPNMLVKEYDOWN)lParam;
                if (pnkd->wVKey == VK_DELETE) {
                    pThis->OnDeleteSelected();
                    return 0;
                }
            } else if (pnm->code == NM_DBLCLK) {
                int sel = ListView_GetNextItem(pThis->m_hListView, -1, LVNI_SELECTED);
                if (sel != -1) {
                    LVITEM lvi = { 0 };
                    lvi.iItem = sel;
                    lvi.mask = LVIF_PARAM;
                    ListView_GetItem(pThis->m_hListView, &lvi);
                    int id = (int)lvi.lParam;
                    auto item = pThis->m_engine->GetItem(id);
                    if (item) {
                        if (item->status == DownloadStatus::Complete) {
                            wchar_t fullPath[MAX_PATH];
                            PathCombineW(fullPath, item->savePath.c_str(), item->fileName.c_str());
                            ShellExecuteW(hWnd, L"open", fullPath, NULL, NULL, SW_SHOWNORMAL);
                        } else {
                            pThis->OpenDownloadPopup(item);
                        }
                    }
                }
            }
        } else if (pThis && pnm->hwndFrom == pThis->m_hStatusBar) {
            if (pnm->code == NM_CLICK || pnm->code == NM_DBLCLK) {
                pThis->ShowSpeedLimiterDialog();
                return 0;
            }
        } else if (pThis && pnm->hwndFrom == pThis->m_hTreeView) {
            if (pnm->code == TVN_SELCHANGED) {
                LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
                wchar_t text[64] = { 0 };
                TVITEM tvi = { 0 };
                tvi.hItem = pnmtv->itemNew.hItem;
                tvi.mask = TVIF_TEXT;
                tvi.pszText = text;
                tvi.cchTextMax = 64;
                TreeView_GetItem(pThis->m_hTreeView, &tvi);
                pThis->m_currentFilter = text;
                pThis->RefreshListView();
            }
        }
        return 0;
    }

    case WM_CONTEXTMENU: {
        if (pThis && (HWND)wParam == pThis->m_hListView) {
            int sel = ListView_GetNextItem(pThis->m_hListView, -1, LVNI_SELECTED);
            if (sel != -1) {
                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, ID_MENU_FILE_OPEN, L"&Open");
                AppendMenuW(hMenu, MF_STRING, ID_MENU_FILE_OPEN_FOLDER, L"Open &Folder");
                AppendMenuW(hMenu, MF_STRING, ID_MENU_CALCULATE_HASH, L"Calculate &Checksum (MD5/SHA256)...");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, ID_TOOLBAR_RESUME, L"&Resume");
                AppendMenuW(hMenu, MF_STRING, ID_TOOLBAR_STOP, L"&Stop");
                AppendMenuW(hMenu, MF_STRING, ID_MENU_REFRESH_ADDRESS, L"&Refresh Download Address...");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, ID_TOOLBAR_DELETE, L"&Delete\tDel");
                AppendMenuW(hMenu, MF_STRING, ID_TOOLBAR_DELETE_COMP, L"Delete &Completed");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, ID_MENU_SPEED_LIMIT_SETTINGS, L"Speed &Limiter...");

                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                if (pt.x == -1 && pt.y == -1) {
                    RECT rc;
                    ListView_GetItemRect(pThis->m_hListView, sel, &rc, LVIR_BOUNDS);
                    MapWindowPoints(pThis->m_hListView, NULL, (LPPOINT)&rc, 2);
                    pt.x = rc.left + 50;
                    pt.y = rc.top + 10;
                }
                TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
                DestroyMenu(hMenu);
            }
        }
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (!pThis) return 0;

        switch (id) {
        case ID_TRAY_OPEN:
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            break;

        case ID_TRAY_ADD_URL:
        case ID_TOOLBAR_ADD_URL:
        case ID_MENU_TASKS_ADD_URL:
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            pThis->ShowAddUrlDialog();
            break;

        case ID_TOOLBAR_RESUME:
        case ID_MENU_FILE_RESUME:
            pThis->OnResumeSelected();
            break;

        case ID_TOOLBAR_STOP:
        case ID_MENU_FILE_STOP:
            pThis->OnStopSelected();
            break;

        case ID_MENU_REFRESH_ADDRESS:
            pThis->OnRefreshAddress();
            break;

        case ID_MENU_CALCULATE_HASH: {
            int sel = ListView_GetNextItem(pThis->m_hListView, -1, LVNI_SELECTED);
            if (sel != -1) {
                LVITEM lvi = { 0 };
                lvi.iItem = sel;
                lvi.mask = LVIF_PARAM;
                ListView_GetItem(pThis->m_hListView, &lvi);
                auto item = pThis->m_engine->GetItem((int)lvi.lParam);
                if (item) {
                    wchar_t fullPath[MAX_PATH];
                    PathCombineW(fullPath, item->savePath.c_str(), item->fileName.c_str());
                    pThis->ShowChecksumDialog(hWnd, fullPath, item->fileName);
                }
            }
            break;
        }

        case ID_MENU_CLIPBOARD_MONITOR: {
            pThis->m_clipboardMonitoring = !pThis->m_clipboardMonitoring;
            HKEY hKey;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Tafim", 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD dwVal = pThis->m_clipboardMonitoring ? 1 : 0;
                RegSetValueExW(hKey, L"ClipboardMonitor", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(dwVal));
                RegCloseKey(hKey);
            }
            pThis->CreateMenus();
            break;
        }

        case ID_MENU_SHUTDOWN_WHEN_DONE: {
            pThis->m_shutdownWhenDone = !pThis->m_shutdownWhenDone;
            HKEY hKey;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Tafim", 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD dwVal = pThis->m_shutdownWhenDone ? 1 : 0;
                RegSetValueExW(hKey, L"ShutdownWhenDone", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(dwVal));
                RegCloseKey(hKey);
            }
            pThis->CreateMenus();
            break;
        }

        case ID_TRAY_PAUSE_ALL:
        case ID_TOOLBAR_STOP_ALL:
        case ID_MENU_DOWNLOADS_STOP_ALL:
            pThis->OnStopAll();
            break;

        case ID_TRAY_RESUME_ALL: {
            auto items = pThis->m_engine->GetAllItems();
            for (auto& it : items) {
                if (it->status == DownloadStatus::Paused) {
                    pThis->m_engine->ResumeDownload(it->id);
                }
            }
            pThis->RefreshListView();
            break;
        }

        case ID_TOOLBAR_DELETE:
        case ID_MENU_FILE_DELETE:
            pThis->OnDeleteSelected();
            break;

        case ID_TOOLBAR_DELETE_COMP:
        case ID_MENU_FILE_DELETE_COMP:
            pThis->OnDeleteCompleted();
            break;

        case ID_MENU_SPEED_LIMIT_ON:
            pThis->m_engine->SetGlobalSpeedLimit(true, pThis->m_engine->GetGlobalSpeedLimitKBps() > 0 ? pThis->m_engine->GetGlobalSpeedLimitKBps() : 500);
            pThis->UpdateSpeedLimiterStatusBar();
            break;

        case ID_MENU_SPEED_LIMIT_OFF:
            pThis->m_engine->SetGlobalSpeedLimit(false, pThis->m_engine->GetGlobalSpeedLimitKBps());
            pThis->UpdateSpeedLimiterStatusBar();
            break;

        case ID_MENU_DOWNLOADS_SPEED_LIMIT:
        case ID_MENU_SPEED_LIMIT_SETTINGS:
            pThis->ShowSpeedLimiterDialog();
            break;

        case ID_TOOLBAR_OPTIONS:
            pThis->ShowOptionsDialog();
            break;

        case ID_MENU_FILE_OPEN: {
            int sel = ListView_GetNextItem(pThis->m_hListView, -1, LVNI_SELECTED);
            if (sel != -1) {
                LVITEM lvi = { 0 };
                lvi.iItem = sel;
                lvi.mask = LVIF_PARAM;
                ListView_GetItem(pThis->m_hListView, &lvi);
                auto item = pThis->m_engine->GetItem((int)lvi.lParam);
                if (item) {
                    wchar_t fullPath[MAX_PATH];
                    PathCombineW(fullPath, item->savePath.c_str(), item->fileName.c_str());
                    ShellExecuteW(hWnd, L"open", fullPath, NULL, NULL, SW_SHOWNORMAL);
                }
            }
            break;
        }

        case ID_MENU_FILE_OPEN_FOLDER: {
            int sel = ListView_GetNextItem(pThis->m_hListView, -1, LVNI_SELECTED);
            if (sel != -1) {
                LVITEM lvi = { 0 };
                lvi.iItem = sel;
                lvi.mask = LVIF_PARAM;
                ListView_GetItem(pThis->m_hListView, &lvi);
                auto item = pThis->m_engine->GetItem((int)lvi.lParam);
                if (item) {
                    ShellExecuteW(hWnd, L"open", item->savePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
            }
            break;
        }

        case ID_TOOLBAR_SCHEDULER:
            MessageBoxW(hWnd, L"Tafim Download Scheduler: Set queue start/stop schedules and hang up options.", L"Scheduler", MB_OK | MB_ICONINFORMATION);
            break;

        case ID_TOOLBAR_GRABBER:
            MessageBoxW(hWnd, L"Tafim Site Grabber: Download all files/media from a website.", L"Site Grabber", MB_OK | MB_ICONINFORMATION);
            break;

        case ID_TOOLBAR_TELL_FRIEND:
            MessageBoxW(hWnd, L"Share Tafim Internet Download Manager with your friends!", L"Tell a Friend", MB_OK | MB_ICONINFORMATION);
            break;

        case ID_MENU_HELP_ABOUT:
            MessageBoxW(hWnd, L"Tafim Internet Download Manager v2.0\nBuild: Native Win32 C/C++\nHigh Speed Multi-Segmented Downloader\n\nDeveloped for Tafim.", L"About Tafim", MB_OK | MB_ICONINFORMATION);
            break;

        case ID_MENU_REGISTRATION:
            MessageBoxW(hWnd, L"Tafim Internet Download Manager is Fully Registered and Activated.\nLicense: Lifetime Unlimited Edition.", L"Registration", MB_OK | MB_ICONINFORMATION);
            break;

        case ID_TRAY_EXIT:
        case ID_MENU_TASKS_EXIT:
            pThis->m_reallyExit = true;
            DestroyWindow(hWnd);
            break;
        }
        return 0;
    }

    case WM_CLOSE:
        if (pThis && !pThis->m_reallyExit) {
            ShowWindow(hWnd, SW_HIDE);
            pThis->ShowTrayBalloon(L"Tafim Running in Background", L"Tafim is active in your system tray.");
            return 0;
        }
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        RemoveClipboardFormatListener(hWnd);
        KillTimer(hWnd, 10);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
