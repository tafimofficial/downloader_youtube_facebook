#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>
#include <tlhelp32.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")

static void KillTafimProcesses() {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(hSnap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, L"Tafim.exe") == 0 || _wcsicmp(pe.szExeFile, L"IDMan.exe") == 0) {
                    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (hProc) {
                        TerminateProcess(hProc, 0);
                        CloseHandle(hProc);
                    }
                }
            } while (Process32NextW(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
}

static void DeleteDirectoryRecursive(const std::wstring& path) {
    std::wstring searchPath = path + L"\\*.*";
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &ffd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0) {
                std::wstring child = path + L"\\" + ffd.cFileName;
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    DeleteDirectoryRecursive(child);
                } else {
                    SetFileAttributesW(child.c_str(), FILE_ATTRIBUTE_NORMAL);
                    DeleteFileW(child.c_str());
                }
            }
        } while (FindNextFileW(hFind, &ffd));
        FindClose(hFind);
    }
    RemoveDirectoryW(path.c_str());
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    int ans = MessageBoxW(NULL,
        L"Are you sure you want to completely remove Tafim Internet Download Manager and all of its components?",
        L"Tafim IDM Uninstall",
        MB_YESNO | MB_ICONQUESTION);

    if (ans != IDYES) {
        return 0;
    }

    // 1. Stop any running Tafim instances
    KillTafimProcesses();
    Sleep(500);

    // 2. Remove Registry Run Key
    HKEY hRunKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hRunKey) == ERROR_SUCCESS) {
        RegDeleteValueW(hRunKey, L"Tafim");
        RegCloseKey(hRunKey);
    }

    // 3. Remove Registry Uninstall Key
    RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Tafim");

    // 4. Remove Desktop Shortcut
    wchar_t desktopPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) {
        PathCombineW(desktopPath, desktopPath, L"Tafim Internet Download Manager.lnk");
        DeleteFileW(desktopPath);
    }

    // 5. Remove Start Menu Shortcut
    wchar_t startMenuPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, startMenuPath))) {
        wchar_t tafimMenuFolder[MAX_PATH];
        PathCombineW(tafimMenuFolder, startMenuPath, L"Tafim");
        DeleteDirectoryRecursive(tafimMenuFolder);
    }

    // 6. Schedule self-deletion of installed directory
    wchar_t currentExe[MAX_PATH];
    GetModuleFileNameW(NULL, currentExe, MAX_PATH);
    wchar_t installDir[MAX_PATH];
    wcscpy_s(installDir, currentExe);
    PathRemoveFileSpecW(installDir);

    // Command to delete directory after this uninstaller terminates
    wchar_t cmd[1024];
    swprintf_s(cmd, L"/c ping 127.0.0.1 -n 2 > nul & rd /s /q \"%s\"", installDir);
    ShellExecuteW(NULL, L"open", L"cmd.exe", cmd, NULL, SW_HIDE);

    MessageBoxW(NULL,
        L"Tafim Internet Download Manager has been successfully uninstalled from your computer.",
        L"Uninstall Complete",
        MB_OK | MB_ICONINFORMATION);

    return 0;
}
