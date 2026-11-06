#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include "Resource.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")

static HRESULT CreateShortcut(LPCWSTR lpszPathObj, LPCWSTR lpszPathLink, LPCWSTR lpszDesc, LPCWSTR lpszArgs) {
    HRESULT hres;
    IShellLinkW* psl;

    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);
    if (SUCCEEDED(hres)) {
        IPersistFile* ppf;
        psl->SetPath(lpszPathObj);
        psl->SetDescription(lpszDesc);
        if (lpszArgs) psl->SetArguments(lpszArgs);

        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hres)) {
            hres = ppf->Save(lpszPathLink, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return hres;
}

static void CopyDirectoryRecursive(const std::wstring& src, const std::wstring& dst) {
    CreateDirectoryW(dst.c_str(), NULL);
    std::wstring search = src + L"\\*.*";
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search.c_str(), &ffd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0) {
                std::wstring sChild = src + L"\\" + ffd.cFileName;
                std::wstring dChild = dst + L"\\" + ffd.cFileName;
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    CopyDirectoryRecursive(sChild, dChild);
                } else {
                    CopyFileW(sChild.c_str(), dChild.c_str(), FALSE);
                }
            }
        } while (FindNextFileW(hFind, &ffd));
        FindClose(hFind);
    }
}

#define IDR_BIN_TAFIM        201
#define IDR_BIN_UNINSTALL    202
#define IDR_BIN_HOST         203
#define IDR_EXT_MANIFEST     204
#define IDR_EXT_BG           205
#define IDR_EXT_CONTENT      206
#define IDR_EXT_CSS          207
#define IDR_EXT_POPUP_HTML   208
#define IDR_EXT_POPUP_JS     209
#define IDR_EXT_ICON16       210
#define IDR_EXT_ICON48       211
#define IDR_EXT_ICON128      212
#define IDR_EXT_YT_BRIDGE    213

static bool ExtractResourceToFile(int resId, LPCWSTR resType, const std::wstring& outputPath) {
    HRSRC hRes = FindResourceW(NULL, MAKEINTRESOURCEW(resId), resType);
    if (!hRes) return false;
    HGLOBAL hGlobal = LoadResource(NULL, hRes);
    if (!hGlobal) return false;
    DWORD size = SizeofResource(NULL, hRes);
    LPVOID pData = LockResource(hGlobal);
    if (!pData || size == 0) return false;

    wchar_t dir[MAX_PATH];
    wcscpy_s(dir, outputPath.c_str());
    PathRemoveFileSpecW(dir);
    CreateDirectoryW(dir, NULL);

    HANDLE hFile = CreateFileW(outputPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    WriteFile(hFile, pData, size, &written, NULL);
    CloseHandle(hFile);
    return written == size;
}

// Dialog controls
#define IDC_INST_PATH_EDIT 1001
#define IDC_INST_BROWSE    1002
#define IDC_INST_AUTOSTART 1003
#define IDC_INST_DESKTOP   1004
#define IDC_INST_PROGRESS  1005
#define IDC_INST_STATUS    1006
#define IDC_INST_INSTALL   1007
#define IDC_INST_CANCEL    1008

static HWND g_hEditPath, g_hChkAutoStart, g_hChkDesktop, g_hProgress, g_hStatus, g_hBtnInstall;
static HFONT g_hFontNormal, g_hFontBold, g_hFontTitle;

static void PerformInstallation(HWND hWnd) {
    wchar_t installDir[MAX_PATH];
    GetWindowTextW(g_hEditPath, installDir, MAX_PATH);

    bool autostart = (SendMessage(g_hChkAutoStart, BM_GETCHECK, 0, 0) == BST_CHECKED);
    bool desktopIcon = (SendMessage(g_hChkDesktop, BM_GETCHECK, 0, 0) == BST_CHECKED);

    EnableWindow(g_hBtnInstall, FALSE);
    SetWindowTextW(g_hStatus, L"Creating installation directories...");
    SendMessage(g_hProgress, PBM_SETPOS, 20, 0);

    CreateDirectoryW(installDir, NULL);

    // Source directory (where Setup_Tafim.exe is running from)
    wchar_t currentExe[MAX_PATH];
    GetModuleFileNameW(NULL, currentExe, MAX_PATH);
    wchar_t sourceDir[MAX_PATH];
    wcscpy_s(sourceDir, currentExe);
    PathRemoveFileSpecW(sourceDir);

    SetWindowTextW(g_hStatus, L"Extracting application binaries...");
    SendMessage(g_hProgress, PBM_SETPOS, 40, 0);

    // 1. Install Tafim.exe
    wchar_t srcTafim[MAX_PATH], dstTafim[MAX_PATH];
    PathCombineW(srcTafim, sourceDir, L"Tafim.exe");
    PathCombineW(dstTafim, installDir, L"Tafim.exe");
    if (!ExtractResourceToFile(IDR_BIN_TAFIM, RT_RCDATA, dstTafim)) {
        CopyFileW(srcTafim, dstTafim, FALSE);
    }

    // 2. Install IDMan.exe (Copy of Tafim.exe)
    wchar_t dstIdman[MAX_PATH];
    PathCombineW(dstIdman, installDir, L"IDMan.exe");
    CopyFileW(dstTafim, dstIdman, FALSE);

    // 3. Install Uninstall.exe
    wchar_t srcUninst[MAX_PATH], dstUninst[MAX_PATH];
    PathCombineW(srcUninst, sourceDir, L"Uninstall.exe");
    PathCombineW(dstUninst, installDir, L"Uninstall.exe");
    if (!ExtractResourceToFile(IDR_BIN_UNINSTALL, RT_RCDATA, dstUninst)) {
        CopyFileW(srcUninst, dstUninst, FALSE);
    }

    // 4. Install TafimHost.exe (Native Messaging Bridge)
    wchar_t srcHost[MAX_PATH], dstHost[MAX_PATH];
    PathCombineW(srcHost, sourceDir, L"TafimHost.exe");
    PathCombineW(dstHost, installDir, L"TafimHost.exe");
    if (!ExtractResourceToFile(IDR_BIN_HOST, RT_RCDATA, dstHost)) {
        CopyFileW(srcHost, dstHost, FALSE);
    }

    // 5. Install Browser Integration Extension
    SetWindowTextW(g_hStatus, L"Installing browser integration extension...");
    SendMessage(g_hProgress, PBM_SETPOS, 60, 0);

    wchar_t dstExt[MAX_PATH];
    PathCombineW(dstExt, installDir, L"extension");
    CreateDirectoryW(dstExt, NULL);
    wchar_t dstIcons[MAX_PATH];
    PathCombineW(dstIcons, dstExt, L"icons");
    CreateDirectoryW(dstIcons, NULL);

    wchar_t fManifest[MAX_PATH], fBg[MAX_PATH], fContent[MAX_PATH], fCss[MAX_PATH], fPopHtml[MAX_PATH], fPopJs[MAX_PATH];
    wchar_t fI16[MAX_PATH], fI48[MAX_PATH], fI128[MAX_PATH], fYtBridge[MAX_PATH];
    PathCombineW(fManifest, dstExt, L"manifest.json");
    PathCombineW(fBg, dstExt, L"background.js");
    PathCombineW(fContent, dstExt, L"content.js");
    PathCombineW(fCss, dstExt, L"content.css");
    PathCombineW(fPopHtml, dstExt, L"popup.html");
    PathCombineW(fPopJs, dstExt, L"popup.js");
    PathCombineW(fYtBridge, dstExt, L"yt_bridge.js");
    PathCombineW(fI16, dstIcons, L"icon16.png");
    PathCombineW(fI48, dstIcons, L"icon48.png");
    PathCombineW(fI128, dstIcons, L"icon128.png");

    bool extExtracted = ExtractResourceToFile(IDR_EXT_MANIFEST, RT_RCDATA, fManifest);
    if (extExtracted) {
        ExtractResourceToFile(IDR_EXT_BG, RT_RCDATA, fBg);
        ExtractResourceToFile(IDR_EXT_CONTENT, RT_RCDATA, fContent);
        ExtractResourceToFile(IDR_EXT_CSS, RT_RCDATA, fCss);
        ExtractResourceToFile(IDR_EXT_POPUP_HTML, RT_RCDATA, fPopHtml);
        ExtractResourceToFile(IDR_EXT_POPUP_JS, RT_RCDATA, fPopJs);
        ExtractResourceToFile(IDR_EXT_YT_BRIDGE, RT_RCDATA, fYtBridge);
        ExtractResourceToFile(IDR_EXT_ICON16, RT_RCDATA, fI16);
        ExtractResourceToFile(IDR_EXT_ICON48, RT_RCDATA, fI48);
        ExtractResourceToFile(IDR_EXT_ICON128, RT_RCDATA, fI128);
    } else {
        wchar_t srcExt[MAX_PATH];
        PathCombineW(srcExt, sourceDir, L"extension");
        CopyDirectoryRecursive(srcExt, dstExt);
    }

    // Write full path to TafimHost.exe in com.tafim.downloader.json
    wchar_t hostJsonPath[MAX_PATH];
    PathCombineW(hostJsonPath, dstExt, L"com.tafim.downloader.json");
    std::wstring hostExeEscaped = dstHost;
    std::wstring escapedPath;
    for (wchar_t c : hostExeEscaped) {
        if (c == L'\\') escapedPath += L"\\\\";
        else escapedPath += c;
    }
    std::wstring jsonContent = L"{\n  \"name\": \"com.tafim.downloader\",\n  \"description\": \"Tafim Internet Download Manager Host\",\n  \"path\": \"" + escapedPath + L"\",\n  \"type\": \"stdio\",\n  \"allowed_origins\": [ \"chrome-extension://*/*\" ]\n}\n";
    FILE* fp = NULL;
    _wfopen_s(&fp, hostJsonPath, L"w, ccs=UTF-8");
    if (fp) {
        fputws(jsonContent.c_str(), fp);
        fclose(fp);
    }

    // Register Native Messaging Host in Windows Registry for Chrome & Edge
    HKEY hChromeKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Google\\Chrome\\NativeMessagingHosts\\com.tafim.downloader", 0, NULL, 0, KEY_SET_VALUE, NULL, &hChromeKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hChromeKey, NULL, 0, REG_SZ, (const BYTE*)hostJsonPath, (DWORD)((wcslen(hostJsonPath) + 1) * sizeof(wchar_t)));
        RegCloseKey(hChromeKey);
    }
    HKEY hEdgeKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Edge\\NativeMessagingHosts\\com.tafim.downloader", 0, NULL, 0, KEY_SET_VALUE, NULL, &hEdgeKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hEdgeKey, NULL, 0, REG_SZ, (const BYTE*)hostJsonPath, (DWORD)((wcslen(hostJsonPath) + 1) * sizeof(wchar_t)));
        RegCloseKey(hEdgeKey);
    }

    // 6. Shortcuts
    SetWindowTextW(g_hStatus, L"Creating system shortcuts...");
    SendMessage(g_hProgress, PBM_SETPOS, 80, 0);

    if (desktopIcon) {
        wchar_t desktopLnk[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopLnk))) {
            PathCombineW(desktopLnk, desktopLnk, L"Tafim Internet Download Manager.lnk");
            CreateShortcut(dstTafim, desktopLnk, L"Tafim Internet Download Manager", L"");
        }
    }

    wchar_t startMenuPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, startMenuPath))) {
        wchar_t tafimMenuFolder[MAX_PATH];
        PathCombineW(tafimMenuFolder, startMenuPath, L"Tafim");
        CreateDirectoryW(tafimMenuFolder, NULL);
        wchar_t startLnk[MAX_PATH];
        PathCombineW(startLnk, tafimMenuFolder, L"Tafim Internet Download Manager.lnk");
        CreateShortcut(dstTafim, startLnk, L"Tafim Internet Download Manager", L"");
        wchar_t uninstLnk[MAX_PATH];
        PathCombineW(uninstLnk, tafimMenuFolder, L"Uninstall Tafim IDM.lnk");
        CreateShortcut(dstUninst, uninstLnk, L"Uninstall Tafim IDM", L"");
    }

    // 7. Registry Run Key (Auto-start on PC boot)
    if (autostart) {
        HKEY hRunKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hRunKey) == ERROR_SUCCESS) {
            std::wstring runCmd = L"\"" + std::wstring(dstTafim) + L"\" /background";
            RegSetValueExW(hRunKey, L"Tafim", 0, REG_SZ, (const BYTE*)runCmd.c_str(), (DWORD)((runCmd.length() + 1) * sizeof(wchar_t)));
            RegCloseKey(hRunKey);
        }
    }

    // 8. Registry Add/Remove Programs entry
    HKEY hUninstKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Tafim", 0, NULL, 0, KEY_SET_VALUE, NULL, &hUninstKey, NULL) == ERROR_SUCCESS) {
        const wchar_t* dispName = L"Tafim Internet Download Manager";
        const wchar_t* dispVer = L"2.0";
        const wchar_t* pub = L"Tafim Software";
        RegSetValueExW(hUninstKey, L"DisplayName", 0, REG_SZ, (const BYTE*)dispName, (DWORD)((wcslen(dispName) + 1) * sizeof(wchar_t)));
        RegSetValueExW(hUninstKey, L"DisplayVersion", 0, REG_SZ, (const BYTE*)dispVer, (DWORD)((wcslen(dispVer) + 1) * sizeof(wchar_t)));
        RegSetValueExW(hUninstKey, L"Publisher", 0, REG_SZ, (const BYTE*)pub, (DWORD)((wcslen(pub) + 1) * sizeof(wchar_t)));
        RegSetValueExW(hUninstKey, L"DisplayIcon", 0, REG_SZ, (const BYTE*)dstTafim, (DWORD)((wcslen(dstTafim) + 1) * sizeof(wchar_t)));
        std::wstring uninstCmd = L"\"" + std::wstring(dstUninst) + L"\"";
        RegSetValueExW(hUninstKey, L"UninstallString", 0, REG_SZ, (const BYTE*)uninstCmd.c_str(), (DWORD)((uninstCmd.length() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hUninstKey, L"InstallLocation", 0, REG_SZ, (const BYTE*)installDir, (DWORD)((wcslen(installDir) + 1) * sizeof(wchar_t)));
        RegCloseKey(hUninstKey);
    }

    SendMessage(g_hProgress, PBM_SETPOS, 100, 0);
    SetWindowTextW(g_hStatus, L"Installation completed successfully!");

    // Start Tafim in background
    ShellExecuteW(NULL, L"open", dstTafim, L"/background", NULL, SW_SHOWNORMAL);

    MessageBoxW(hWnd,
        L"Tafim Internet Download Manager has been successfully installed!\n\n"
        L"- Tafim is now active in your system tray.\n"
        L"- It will automatically start in the background when your PC turns on.\n"
        L"- Browser Extension is installed and ready at:\n"
        L"  extension/ inside the installation directory.\n\n"
        L"You can now manage and accelerate your downloads!",
        L"Setup Successful",
        MB_OK | MB_ICONINFORMATION);

    DestroyWindow(hWnd);
}

static LRESULT CALLBACK SetupWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HINSTANCE hInst = ((CREATESTRUCT*)lParam)->hInstance;

        g_hFontNormal = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_hFontBold = CreateFontW(-12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_hFontTitle = CreateFontW(-15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        // Top Banner Text
        HWND hTitle = CreateWindowExW(0, L"STATIC", L"Install Tafim Internet Download Manager 2.0",
            WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 16, 460, 24, hWnd, NULL, hInst, NULL);
        SendMessage(hTitle, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);

        HWND hSub = CreateWindowExW(0, L"STATIC", L"The setup wizard will install Tafim on your computer.",
            WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 42, 460, 18, hWnd, NULL, hInst, NULL);
        SendMessage(hSub, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        // Path Label
        HWND hLblPath = CreateWindowExW(0, L"STATIC", L"Destination Folder:",
            WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 80, 200, 18, hWnd, NULL, hInst, NULL);
        SendMessage(hLblPath, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        // Default path: %LOCALAPPDATA%\Programs\Tafim
        wchar_t defPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, defPath))) {
            PathCombineW(defPath, defPath, L"Programs\\Tafim");
        } else {
            wcscpy_s(defPath, L"C:\\Program Files\\Tafim");
        }

        g_hEditPath = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", defPath,
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 20, 102, 380, 24, hWnd, (HMENU)IDC_INST_PATH_EDIT, hInst, NULL);
        SendMessage(g_hEditPath, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        // Options
        g_hChkAutoStart = CreateWindowExW(0, L"BUTTON", L"Start Tafim automatically when Windows starts (Background mode)",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 20, 142, 450, 22, hWnd, (HMENU)IDC_INST_AUTOSTART, hInst, NULL);
        SendMessage(g_hChkAutoStart, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessage(g_hChkAutoStart, BM_SETCHECK, BST_CHECKED, 0);

        g_hChkDesktop = CreateWindowExW(0, L"BUTTON", L"Create Desktop shortcut icon",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 20, 168, 300, 22, hWnd, (HMENU)IDC_INST_DESKTOP, hInst, NULL);
        SendMessage(g_hChkDesktop, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessage(g_hChkDesktop, BM_SETCHECK, BST_CHECKED, 0);

        // Progress bar & Status
        g_hProgress = CreateWindowExW(0, PROGRESS_CLASS, L"",
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 20, 208, 475, 18, hWnd, (HMENU)IDC_INST_PROGRESS, hInst, NULL);
        SendMessage(g_hProgress, PBM_SETRANGE32, 0, 100);

        g_hStatus = CreateWindowExW(0, L"STATIC", L"Ready to install.",
            WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 234, 475, 18, hWnd, (HMENU)IDC_INST_STATUS, hInst, NULL);
        SendMessage(g_hStatus, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        // Action Buttons
        g_hBtnInstall = CreateWindowExW(0, L"BUTTON", L"Install",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 300, 270, 95, 28, hWnd, (HMENU)IDC_INST_INSTALL, hInst, NULL);
        SendMessage(g_hBtnInstall, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 405, 270, 90, 28, hWnd, (HMENU)IDC_INST_CANCEL, hInst, NULL);
        SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        // Top Header Banner
        RECT rcBanner = { 0, 0, rc.right, 70 };
        HBRUSH bannerBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rcBanner, bannerBrush);
        DeleteObject(bannerBrush);

        HPEN linePen = CreatePen(PS_SOLID, 1, RGB(220, 224, 230));
        HPEN oldPen = (HPEN)SelectObject(hdc, linePen);
        MoveToEx(hdc, 0, 70, NULL);
        LineTo(hdc, rc.right, 70);
        SelectObject(hdc, oldPen);
        DeleteObject(linePen);

        // Inset card for options
        RECT rcBottomLine = { 0, 258, rc.right, 259 };
        HBRUSH lineBrush = CreateSolidBrush(RGB(220, 224, 230));
        FillRect(hdc, &rcBottomLine, lineBrush);
        DeleteObject(lineBrush);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_INST_INSTALL) {
            PerformInstallation(hWnd);
        } else if (id == IDC_INST_CANCEL) {
            DestroyWindow(hWnd);
        }
        return 0;
    }

    case WM_DESTROY:
        if (g_hFontNormal) DeleteObject(g_hFontNormal);
        if (g_hFontBold) DeleteObject(g_hFontBold);
        if (g_hFontTitle) DeleteObject(g_hFontTitle);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    CoInitialize(NULL);

    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_WIN95_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&iccex);

    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = SetupWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"TafimSetupWnd";
    wc.hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    RegisterClassEx(&wc);

    int winW = 530;
    int winH = 350;
    int x = (GetSystemMetrics(SM_CXSCREEN) - winW) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - winH) / 2;

    HWND hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        L"TafimSetupWnd",
        L"Tafim Internet Download Manager 2.0 Setup",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, winW, winH,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();
    return 0;
}
