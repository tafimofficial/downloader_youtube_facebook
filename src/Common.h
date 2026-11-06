#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <winhttp.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <winioctl.h>
#include <wincrypt.h>

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ws2_32.lib")

// Custom Window Messages
#define WM_APP_DOWNLOAD_PROGRESS (WM_APP + 101)
#define WM_APP_DOWNLOAD_FINISHED (WM_APP + 102)
#define WM_APP_DOWNLOAD_STATUS   (WM_APP + 103)
#define WM_APP_START_DOWNLOAD    (WM_APP + 104)
#define WM_APP_TRAY_MSG          (WM_APP + 201)
#define WM_APP_EXT_DOWNLOAD      (WM_APP + 202)
#define WC_MAINWINDOW            L"TafimMainWindow"

// Helper formatting functions
inline std::wstring FormatFileSize(uint64_t bytes) {
    wchar_t buf[64];
    if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
        double gb = (double)bytes / (1024.0 * 1024.0 * 1024.0);
        swprintf_s(buf, L"%.2f GB", gb);
    } else if (bytes >= 1024ULL * 1024ULL) {
        double mb = (double)bytes / (1024.0 * 1024.0);
        swprintf_s(buf, L"%.2f MB", mb);
    } else if (bytes >= 1024ULL) {
        double kb = (double)bytes / 1024.0;
        swprintf_s(buf, L"%.2f KB", kb);
    } else {
        swprintf_s(buf, L"%llu Bytes", bytes);
    }
    return buf;
}

inline std::wstring FormatSpeed(double bytesPerSec) {
    wchar_t buf[64];
    if (bytesPerSec >= 1024.0 * 1024.0) {
        swprintf_s(buf, L"%.2f MB/sec", bytesPerSec / (1024.0 * 1024.0));
    } else if (bytesPerSec >= 1024.0) {
        swprintf_s(buf, L"%.2f KB/sec", bytesPerSec / 1024.0);
    } else {
        swprintf_s(buf, L"%.0f B/sec", bytesPerSec);
    }
    return buf;
}

inline std::wstring FormatTimeLeft(double seconds) {
    if (seconds < 0 || seconds > 86400 * 30) return L"--";
    int s = (int)seconds;
    int h = s / 3600;
    int m = (s % 3600) / 60;
    int sec = s % 60;
    wchar_t buf[64];
    if (h > 0) {
        swprintf_s(buf, L"%d hr %d min", h, m);
    } else if (m > 0) {
        swprintf_s(buf, L"%d min %d sec", m, sec);
    } else {
        swprintf_s(buf, L"%d sec", sec);
    }
    return buf;
}

inline bool CalculateFileChecksums(const std::wstring& filePath, std::wstring& md5Out, std::wstring& sha256Out) {
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    HCRYPTPROV hProv = 0;
    if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            CloseHandle(hFile);
            return false;
        }
    }

    HCRYPTHASH hHashMD5 = 0, hHashSHA256 = 0;
    CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHashMD5);
    CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHashSHA256);

    std::vector<BYTE> buffer(1024 * 1024); // 1 MB streaming buffer
    DWORD bytesRead = 0;
    while (ReadFile(hFile, buffer.data(), (DWORD)buffer.size(), &bytesRead, NULL) && bytesRead > 0) {
        if (hHashMD5) CryptHashData(hHashMD5, buffer.data(), bytesRead, 0);
        if (hHashSHA256) CryptHashData(hHashSHA256, buffer.data(), bytesRead, 0);
    }
    CloseHandle(hFile);

    auto hashToHex = [](HCRYPTHASH hHash) -> std::wstring {
        if (!hHash) return L"";
        DWORD hashLen = 0;
        DWORD dwSize = sizeof(hashLen);
        CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashLen, &dwSize, 0);
        std::vector<BYTE> hashData(hashLen);
        CryptGetHashParam(hHash, HP_HASHVAL, hashData.data(), &hashLen, 0);
        std::wstringstream ss;
        for (BYTE b : hashData) {
            ss << std::hex << std::setw(2) << std::setfill(L'0') << (int)b;
        }
        return ss.str();
    };

    md5Out = hashToHex(hHashMD5);
    sha256Out = hashToHex(hHashSHA256);

    if (hHashMD5) CryptDestroyHash(hHashMD5);
    if (hHashSHA256) CryptDestroyHash(hHashSHA256);
    if (hProv) CryptReleaseContext(hProv, 0);
    return true;
}
