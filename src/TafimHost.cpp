#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shlwapi.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include <vector>
#include <io.h>
#include <fcntl.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "winhttp.lib")

static bool SendHttpDownload(const std::string& jsonPayload) {
    HINTERNET hSession = WinHttpOpen(L"TafimHost/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    DWORD timeout = 2000;
    WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));

    HINTERNET hConnect = WinHttpConnect(hSession, L"127.0.0.1", 9898, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/download", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    LPCWSTR hdrs = L"Content-Type: application/json\r\n";
    BOOL sent = WinHttpSendRequest(hRequest, hdrs, (DWORD)wcslen(hdrs),
        (LPVOID)jsonPayload.data(), (DWORD)jsonPayload.length(), (DWORD)jsonPayload.length(), 0);

    bool ok = false;
    if (sent && WinHttpReceiveResponse(hRequest, NULL)) {
        DWORD status = 0;
        DWORD stSize = sizeof(status);
        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &stSize, WINHTTP_NO_HEADER_INDEX)) {
            ok = (status == 200);
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ok;
}

static void LaunchTafimExe() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    wchar_t tafimExe[MAX_PATH];
    PathCombineW(tafimExe, exePath, L"Tafim.exe");

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    std::wstring cmd = L"\"" + std::wstring(tafimExe) + L"\" /background";
    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back(0);

    if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!hStdIn || hStdIn == INVALID_HANDLE_VALUE || !hStdOut || hStdOut == INVALID_HANDLE_VALUE) {
        return 0;
    }

    uint32_t length = 0;
    DWORD bytesRead = 0;
    if (!ReadFile(hStdIn, &length, 4, &bytesRead, NULL) || bytesRead != 4 || length == 0 || length > 1024 * 1024) {
        return 0;
    }

    std::string message(length, '\0');
    DWORD totalRead = 0;
    while (totalRead < length) {
        DWORD readNow = 0;
        if (!ReadFile(hStdIn, &message[totalRead], length - totalRead, &readNow, NULL) || readNow == 0) {
            return 0;
        }
        totalRead += readNow;
    }

    // 1. Try forwarding JSON message to running Tafim.exe HTTP API
    bool dispatched = SendHttpDownload(message);

    // 2. If Tafim is not running, launch Tafim.exe and retry
    if (!dispatched) {
        LaunchTafimExe();
        for (int i = 0; i < 15; i++) {
            Sleep(200);
            if (SendHttpDownload(message)) {
                dispatched = true;
                break;
            }
        }
    }

    // Send response back to Chrome Native Messaging
    std::string response = dispatched ? "{\"success\":true}" : "{\"success\":false}";
    uint32_t respLen = (uint32_t)response.length();
    DWORD written = 0;
    WriteFile(hStdOut, &respLen, 4, &written, NULL);
    WriteFile(hStdOut, response.c_str(), respLen, &written, NULL);

    return 0;
}
