#include "Common.h"
#include "MainWindow.h"
#include <gdiplus.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Local\\Tafim_IDM_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hExisting = FindWindowW(WC_MAINWINDOW, NULL);
        if (hExisting) {
            if (pCmdLine && (wcsstr(pCmdLine, L"/background") != NULL || wcsstr(pCmdLine, L"-background") != NULL)) {
                if (hMutex) CloseHandle(hMutex);
                return 0;
            }
            ShowWindow(hExisting, SW_RESTORE);
            SetForegroundWindow(hExisting);
            if (hMutex) CloseHandle(hMutex);
            return 0;
        }
    }

    // 1. Initialize COM
    HRESULT hr = CoInitialize(NULL);

    // 2. Initialize Common Controls
    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_WIN95_CLASSES | ICC_PROGRESS_CLASS | ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&iccex);

    // 3. Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // 4. Create and run Main Window
    MainWindow* pMain = MainWindow::Create(hInstance, pCmdLine ? pCmdLine : L"");
    int exitCode = 0;
    if (pMain) {
        exitCode = pMain->Run();
        delete pMain;
    }

    // 5. Cleanup
    Gdiplus::GdiplusShutdown(gdiplusToken);
    CoUninitialize();

    return exitCode;
}
