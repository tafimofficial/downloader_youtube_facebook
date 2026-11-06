#include "DownloadEngine.h"

static std::wstring LocateFfmpeg() {
    // 1. Current application directory
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    wchar_t localFfmpeg[MAX_PATH];
    PathCombineW(localFfmpeg, exePath, L"ffmpeg.exe");
    if (GetFileAttributesW(localFfmpeg) != INVALID_FILE_ATTRIBUTES) {
        return localFfmpeg;
    }

    // 2. SearchPathW in system PATH
    wchar_t pathBuf[MAX_PATH] = { 0 };
    if (SearchPathW(NULL, L"ffmpeg.exe", NULL, MAX_PATH, pathBuf, NULL) > 0) {
        return pathBuf;
    }

    // 3. WinGet package directory
    wchar_t localAppData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
        std::wstring wingetFfmpeg = std::wstring(localAppData) + L"\\Microsoft\\WinGet\\Packages\\Gyan.FFmpeg_Microsoft.Winget.Source_8wekyb3d8bbwe\\ffmpeg-9.0.2-full_build\\bin\\ffmpeg.exe";
        if (GetFileAttributesW(wingetFfmpeg.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return wingetFfmpeg;
        }
    }

    // 4. Fallback to system PATH
    return L"ffmpeg.exe";
}

static bool MergeDashStreams(const std::wstring& videoPath, const std::wstring& audioPath, const std::wstring& outputPath) {
    std::wstring ffmpeg = LocateFfmpeg();

    // Strategy 1: Fast direct stream copy (no transcoding, ~1 second)
    std::wstring cmd = L"\"" + ffmpeg + L"\" -y -i \"" + videoPath + L"\" -i \"" + audioPath + L"\" -c copy -movflags +faststart \"" + outputPath + L"\"";
    
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = { 0 };
    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back(0);

    BOOL ok = CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (ok) {
        WaitForSingleObject(pi.hProcess, 60000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        HANDLE hFile = CreateFileW(outputPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER sz;
            GetFileSizeEx(hFile, &sz);
            CloseHandle(hFile);
            if (sz.QuadPart > 1024) {
                DeleteFileW(videoPath.c_str());
                DeleteFileW(audioPath.c_str());
                return true;
            }
        }
    }

    // Strategy 2: In case audio stream is WebM/Opus and container is MP4, transcode audio to standard AAC
    std::wstring cmd2 = L"\"" + ffmpeg + L"\" -y -i \"" + videoPath + L"\" -i \"" + audioPath + L"\" -c:v copy -c:a aac -b:a 192k -movflags +faststart \"" + outputPath + L"\"";
    std::vector<wchar_t> cmdBuf2(cmd2.begin(), cmd2.end());
    cmdBuf2.push_back(0);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    ok = CreateProcessW(NULL, cmdBuf2.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (ok) {
        WaitForSingleObject(pi.hProcess, 120000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        HANDLE hFile = CreateFileW(outputPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER sz;
            GetFileSizeEx(hFile, &sz);
            CloseHandle(hFile);
            if (sz.QuadPart > 1024) {
                DeleteFileW(videoPath.c_str());
                DeleteFileW(audioPath.c_str());
                return true;
            }
        }
    }

    // Fallback: preserve video file so user doesn't lose the downloaded stream
    MoveFileExW(videoPath.c_str(), outputPath.c_str(), MOVEFILE_REPLACE_EXISTING);
    DeleteFileW(audioPath.c_str());
    return true;
}

static bool IsYouTubeWatchUrl(const std::wstring& url) {
    std::wstring u = url;
    std::transform(u.begin(), u.end(), u.begin(), ::towlower);
    return (u.find(L"youtube.com/watch") != std::wstring::npos ||
            u.find(L"youtu.be/") != std::wstring::npos ||
            u.find(L"youtube.com/shorts") != std::wstring::npos ||
            u.find(L"youtube.com/live") != std::wstring::npos);
}

static bool IsWebVideoPageUrl(const std::wstring& url) {
    std::wstring u = url;
    std::transform(u.begin(), u.end(), u.begin(), ::towlower);
    return IsYouTubeWatchUrl(url) ||
           u.find(L"facebook.com/") != std::wstring::npos ||
           u.find(L"fb.watch/") != std::wstring::npos ||
           u.find(L"instagram.com/") != std::wstring::npos ||
           u.find(L"tiktok.com/") != std::wstring::npos ||
           u.find(L"twitter.com/") != std::wstring::npos ||
           u.find(L"x.com/") != std::wstring::npos ||
           u.find(L"vimeo.com/") != std::wstring::npos ||
           u.find(L"reddit.com/") != std::wstring::npos ||
           u.find(L"dailymotion.com/") != std::wstring::npos;
}

static bool IsGoogleVideoUrl(const std::wstring& url) {
    return url.find(L"googlevideo.com") != std::wstring::npos;
}

static bool IsGoogleDriveUrl(const std::wstring& url) {
    std::wstring u = url;
    std::transform(u.begin(), u.end(), u.begin(), ::towlower);
    return u.find(L"drive.google.com") != std::wstring::npos ||
           u.find(L"drive.usercontent.google.com") != std::wstring::npos ||
           u.find(L"docs.google.com") != std::wstring::npos;
}

static bool IsCloudOrProtectedUrl(const std::wstring& url) {
    std::wstring u = url;
    std::transform(u.begin(), u.end(), u.begin(), ::towlower);
    return IsGoogleDriveUrl(url) ||
           u.find(L"oaiusercontent.com") != std::wstring::npos ||
           u.find(L"chatgpt.com") != std::wstring::npos ||
           u.find(L"dropbox.com") != std::wstring::npos ||
           u.find(L"mediafire.com") != std::wstring::npos ||
           u.find(L"1drv.ms") != std::wstring::npos ||
           u.find(L"onedrive.live.com") != std::wstring::npos;
}

static std::wstring NormalizeGoogleDriveUrl(const std::wstring& inUrl) {
    std::wstring u = inUrl;
    size_t fileD = u.find(L"/file/d/");
    if (fileD != std::wstring::npos) {
        size_t idStart = fileD + 8;
        size_t idEnd = u.find_first_of(L"/?#", idStart);
        std::wstring id = (idEnd == std::wstring::npos) ? u.substr(idStart) : u.substr(idStart, idEnd - idStart);
        if (!id.empty()) {
            return L"https://drive.usercontent.google.com/download?id=" + id + L"&export=download";
        }
    }
    size_t openId = u.find(L"open?id=");
    if (openId != std::wstring::npos) {
        size_t idStart = openId + 8;
        size_t idEnd = u.find_first_of(L"&#", idStart);
        std::wstring id = (idEnd == std::wstring::npos) ? u.substr(idStart) : u.substr(idStart, idEnd - idStart);
        if (!id.empty()) {
            return L"https://drive.usercontent.google.com/download?id=" + id + L"&export=download";
        }
    }
    return inUrl;
}

static std::wstring ExtractFilenameFromContentDisposition(const std::wstring& cd) {
    if (cd.empty()) return L"";

    // 1. filename*=UTF-8''...
    size_t fnStar = cd.find(L"filename*=");
    if (fnStar != std::wstring::npos) {
        size_t valStart = fnStar + 10;
        size_t utf8Pos = cd.find(L"UTF-8''", valStart);
        if (utf8Pos != std::wstring::npos) {
            valStart = utf8Pos + 7;
        }
        size_t valEnd = cd.find(L';', valStart);
        std::wstring encoded = (valEnd == std::wstring::npos) ? cd.substr(valStart) : cd.substr(valStart, valEnd - valStart);
        while (!encoded.empty() && (encoded.front() == L'"' || encoded.front() == L' ' || encoded.front() == L'\'')) encoded.erase(0, 1);
        while (!encoded.empty() && (encoded.back() == L'"' || encoded.back() == L' ' || encoded.back() == L'\'')) encoded.pop_back();

        std::string s(encoded.begin(), encoded.end());
        std::string decoded;
        for (size_t i = 0; i < s.length(); i++) {
            if (s[i] == '%' && i + 2 < s.length()) {
                int ch = 0;
                if (sscanf_s(s.substr(i + 1, 2).c_str(), "%x", &ch) == 1) {
                    decoded += (char)ch;
                    i += 2;
                    continue;
                }
            }
            decoded += s[i];
        }
        int wlen = MultiByteToWideChar(CP_UTF8, 0, decoded.c_str(), -1, NULL, 0);
        if (wlen > 1) {
            std::wstring res(wlen, 0);
            MultiByteToWideChar(CP_UTF8, 0, decoded.c_str(), -1, &res[0], wlen);
            while (!res.empty() && res.back() == L'\0') res.pop_back();
            if (!res.empty()) return res;
        }
    }

    // 2. filename="..." or filename=...
    size_t fn = cd.find(L"filename=");
    if (fn != std::wstring::npos) {
        size_t valStart = fn + 9;
        while (valStart < cd.length() && cd[valStart] == L' ') valStart++;
        if (valStart < cd.length() && cd[valStart] == L'"') {
            valStart++;
            size_t valEnd = cd.find(L'"', valStart);
            if (valEnd != std::wstring::npos) {
                return cd.substr(valStart, valEnd - valStart);
            }
        } else {
            size_t valEnd = cd.find_first_of(L"; \r\n", valStart);
            return (valEnd == std::wstring::npos) ? cd.substr(valStart) : cd.substr(valStart, valEnd - valStart);
        }
    }
    return L"";
}

static std::wstring SanitizeFilename(std::wstring name) {
    const wchar_t invalidChars[] = L"\\/:*?\"<>|";
    for (wchar_t c : invalidChars) {
        std::replace(name.begin(), name.end(), c, L'_');
    }
    while (!name.empty() && (name.front() == L' ' || name.front() == L'.')) name.erase(0, 1);
    while (!name.empty() && (name.back() == L' ' || name.back() == L'.')) name.pop_back();
    return name.empty() ? L"download.bin" : name;
}

static void UpdateItemCategoryFromExt(DownloadItem* item) {
    if (!item) return;
    std::wstring ext = PathFindExtensionW(item->fileName.c_str());
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    if (ext == L".zip" || ext == L".rar" || ext == L".7z" || ext == L".tar" || ext == L".gz") {
        item->category = L"Compressed";
    } else if (ext == L".exe" || ext == L".msi" || ext == L".bat" || ext == L".cmd") {
        item->category = L"Programs";
    } else if (ext == L".mp4" || ext == L".mkv" || ext == L".avi" || ext == L".webm" || ext == L".ogv" || ext == L".mov") {
        item->category = L"Video";
    } else if (ext == L".mp3" || ext == L".wav" || ext == L".flac" || ext == L".aac" || ext == L".ogg") {
        item->category = L"Music";
    } else if (ext == L".pdf" || ext == L".doc" || ext == L".docx" || ext == L".txt" || ext == L".epub" || ext == L".xlsx" || ext == L".pptx") {
        item->category = L"Documents";
    }
}

static void RemoveQueryParam(std::wstring& url, const std::wstring& key) {
    std::wstring needle = key + L"=";
    size_t pos = 0;
    while ((pos = url.find(needle, pos)) != std::wstring::npos) {
        if (pos > 0 && url[pos - 1] != L'?' && url[pos - 1] != L'&') {
            pos += needle.size();
            continue;
        }
        size_t end = url.find(L'&', pos);
        if (end == std::wstring::npos) {
            url.erase(pos);
            while (!url.empty() && (url.back() == L'?' || url.back() == L'&')) url.pop_back();
            break;
        }
        url.erase(pos, end - pos + 1);
    }
}

static std::wstring SanitizeGoogleVideoUrl(std::wstring url) {
    const wchar_t* keys[] = {
        L"range", L"rn", L"rbuf", L"ump", L"sq", L"alr", L"srfvp",
        L"begin", L"len", L"bytestart", L"byteend", L"cmo"
    };
    for (auto k : keys) RemoveQueryParam(url, k);
    return url;
}

static std::wstring LocateYtDlp() {
    // 1. Current application directory
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    wchar_t local[MAX_PATH];
    PathCombineW(local, exePath, L"yt-dlp.exe");
    if (GetFileAttributesW(local) != INVALID_FILE_ATTRIBUTES) {
        return local;
    }

    // 2. System PATH via SearchPathW
    wchar_t pathBuf[MAX_PATH] = { 0 };
    if (SearchPathW(NULL, L"yt-dlp.exe", NULL, MAX_PATH, pathBuf, NULL) > 0) {
        return pathBuf;
    }

    // 3. Check Python scripts directories in LocalAppData
    wchar_t localAppData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
        std::wstring pyDir = std::wstring(localAppData) + L"\\Programs\\Python";
        std::wstring searchSub = pyDir + L"\\Python*";
        WIN32_FIND_DATAW ffd;
        HANDLE hSub = FindFirstFileW(searchSub.c_str(), &ffd);
        if (hSub != INVALID_HANDLE_VALUE) {
            do {
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    std::wstring candidate = pyDir + L"\\" + ffd.cFileName + L"\\Scripts\\yt-dlp.exe";
                    if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        FindClose(hSub);
                        return candidate;
                    }
                }
            } while (FindNextFileW(hSub, &ffd));
            FindClose(hSub);
        }
    }

    return L"";
}

class DownloadTask {
public:
    DownloadTask(std::shared_ptr<DownloadItem> item, IDownloadCallback* cb)
        : m_item(item), m_callback(cb), m_isPaused(false), m_isCanceled(false), m_isRunning(false), m_hChildProcess(NULL) {}

    ~DownloadTask() {
        Stop();
    }

    void Start() {
        if (m_isRunning) return;
        if (m_mainThread.joinable()) {
            m_mainThread.join();
        }
        m_isPaused = false;
        m_isCanceled = false;
        m_isRunning = true;
        m_mainThread = std::thread(&DownloadTask::Run, this);
    }

    void Pause() {
        m_isPaused = true;
        if (m_hChildProcess) {
            TerminateProcess(m_hChildProcess, 0);
            CloseHandle(m_hChildProcess);
            m_hChildProcess = NULL;
        }
        if (m_item) {
            m_item->status = DownloadStatus::Paused;
            m_item->speedBytesPerSec = 0;
            m_item->timeLeftSec = 0;
            if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Paused);
        }
    }

    void Cancel() {
        m_isCanceled = true;
        if (m_hChildProcess) {
            TerminateProcess(m_hChildProcess, 1);
            CloseHandle(m_hChildProcess);
            m_hChildProcess = NULL;
        }
        if (m_item) {
            m_item->status = DownloadStatus::Canceled;
            m_item->speedBytesPerSec = 0;
            m_item->timeLeftSec = 0;
            if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Canceled);
        }
    }

    void Stop() {
        m_isPaused = true;
        m_isCanceled = true;
        if (m_hChildProcess) {
            TerminateProcess(m_hChildProcess, 1);
            CloseHandle(m_hChildProcess);
            m_hChildProcess = NULL;
        }
        if (m_mainThread.joinable()) {
            m_mainThread.join();
        }
        for (auto& t : m_workerThreads) {
            if (t.joinable()) t.join();
        }
        m_workerThreads.clear();
        m_isRunning = false;
    }

    bool IsRunning() const { return m_isRunning; }
    int GetId() const { return m_item ? m_item->id : 0; }

private:
    std::shared_ptr<DownloadItem> m_item;
    IDownloadCallback* m_callback;
    std::atomic<bool> m_isPaused;
    std::atomic<bool> m_isCanceled;
    std::atomic<bool> m_isRunning;
    HANDLE m_hChildProcess;
    std::thread m_mainThread;
    std::vector<std::thread> m_workerThreads;

    bool DownloadSingleStreamDirect(const std::wstring& streamUrl, const std::wstring& tempPath, uint64_t& outTotalBytes, int connIndex, const std::wstring& label, uint64_t resumeOffset = 0, bool forceOffset = false) {
        std::wstring fetchUrl = IsGoogleVideoUrl(streamUrl) ? SanitizeGoogleVideoUrl(streamUrl) : streamUrl;
        if (IsGoogleDriveUrl(fetchUrl)) {
            fetchUrl = NormalizeGoogleDriveUrl(fetchUrl);
        }

        std::wstring actualUA = !m_item->userAgent.empty() ? m_item->userAgent : L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/155.0.0.0 Safari/537.36";
        HINTERNET hSession = WinHttpOpen(actualUA.c_str(),
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return false;

        DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        WinHttpSetOption(hSession, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

        DWORD secureProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
        WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secureProtocols, sizeof(secureProtocols));
        WinHttpSetTimeouts(hSession, 15000, 20000, 30000, 30000);

        DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                         SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;

        uint64_t startByte = resumeOffset;
        if (!forceOffset && startByte == 0) {
            WIN32_FILE_ATTRIBUTE_DATA fad;
            if (GetFileAttributesExW(tempPath.c_str(), GetFileExInfoStandard, &fad)) {
                startByte = ((uint64_t)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
            }
        }

        int redirectHops = 0;
        HINTERNET hConnect = NULL;
        HINTERNET hRequest = NULL;
        DWORD statusCode = 0;
        bool triedUAFallback = false;

        while (redirectHops < 10 && !m_isCanceled) {
            redirectHops++;
            URL_COMPONENTS urlComp = { 0 };
            urlComp.dwStructSize = sizeof(urlComp);
            std::vector<wchar_t> hostName(2048, 0);
            std::vector<wchar_t> urlPath(32768, 0);
            urlComp.lpszHostName = hostName.data();
            urlComp.dwHostNameLength = (DWORD)hostName.size();
            urlComp.lpszUrlPath = urlPath.data();
            urlComp.dwUrlPathLength = (DWORD)urlPath.size();

            if (!WinHttpCrackUrl(fetchUrl.c_str(), (DWORD)fetchUrl.length(), 0, &urlComp)) {
                if (connIndex < (int)m_item->connections.size()) m_item->connections[connIndex].status = L"Invalid URL";
                WinHttpCloseHandle(hSession);
                return false;
            }

            hConnect = WinHttpConnect(hSession, hostName.data(), urlComp.nPort, 0);
            if (!hConnect) {
                WinHttpCloseHandle(hSession);
                return false;
            }

            DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
            hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath.data(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
            if (!hRequest) {
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return false;
            }

            DWORD http2Flag = WINHTTP_PROTOCOL_FLAG_HTTP2;
            WinHttpSetOption(hRequest, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, &http2Flag, sizeof(http2Flag));
            WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
            WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

            std::wstring browserHdrs = L"User-Agent: " + actualUA + L"\r\n"
                L"Accept: */*\r\n"
                L"Accept-Language: en-US,en;q=0.9\r\n";
            if (!m_item->referer.empty()) {
                browserHdrs += L"Referer: " + m_item->referer + L"\r\n";
            } else if (IsGoogleVideoUrl(fetchUrl)) {
                browserHdrs += L"Referer: https://www.youtube.com/\r\n";
            }
            if (IsGoogleVideoUrl(fetchUrl)) {
                browserHdrs += L"Origin: https://www.youtube.com\r\n"
                               L"Sec-Fetch-Dest: video\r\n"
                               L"Sec-Fetch-Mode: cors\r\n"
                               L"Sec-Fetch-Site: cross-site\r\n";
            }
            if (!m_item->cookies.empty()) {
                browserHdrs += L"Cookie: " + m_item->cookies + L"\r\n";
            }
            if (startByte > 0) {
                wchar_t rangeHdr[64];
                swprintf_s(rangeHdr, L"Range: bytes=%llu-\r\n", startByte);
                browserHdrs += rangeHdr;
            }

            WinHttpAddRequestHeaders(hRequest, browserHdrs.c_str(), (DWORD)browserHdrs.length(), WINHTTP_ADDREQ_FLAG_ADD);

            if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
                !WinHttpReceiveResponse(hRequest, NULL)) {
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return false;
            }

            statusCode = 0;
            DWORD scSize = sizeof(statusCode);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &scSize, WINHTTP_NO_HEADER_INDEX);

            // Bypass Cloudflare/WAF block when browser UA is blocked on non-browser TLS
            if ((statusCode == 403 || statusCode == 406) && !triedUAFallback) {
                triedUAFallback = true;
                actualUA = L"curl/8.4.0";
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                hSession = WinHttpOpen(actualUA.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
                if (!hSession) return false;
                WinHttpSetOption(hSession, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));
                WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secureProtocols, sizeof(secureProtocols));
                WinHttpSetTimeouts(hSession, 15000, 20000, 30000, 30000);
                continue;
            }

            if (statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 307 || statusCode == 308) {
                std::vector<wchar_t> locHeader(8192, 0);
                DWORD locSize = (DWORD)(locHeader.size() * sizeof(wchar_t));
                if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX, locHeader.data(), &locSize, WINHTTP_NO_HEADER_INDEX)) {
                    std::vector<wchar_t> combBuf(8192, 0);
                    DWORD combLen = (DWORD)combBuf.size();
                    if (SUCCEEDED(UrlCombineW(fetchUrl.c_str(), locHeader.data(), combBuf.data(), &combLen, 0))) {
                        fetchUrl = combBuf.data();
                    } else {
                        fetchUrl = locHeader.data();
                    }
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(hConnect);
                    continue;
                }
            }

            // Check Content-Disposition for filename
            wchar_t cdHeader[1024] = { 0 };
            DWORD cdSize = sizeof(cdHeader);
            if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_DISPOSITION, WINHTTP_HEADER_NAME_BY_INDEX, cdHeader, &cdSize, WINHTTP_NO_HEADER_INDEX)) {
                std::wstring parsed = ExtractFilenameFromContentDisposition(cdHeader);
                if (!parsed.empty()) {
                    std::wstring sanitized = SanitizeFilename(parsed);
                    if (m_item->fileName.empty() || m_item->fileName == L"download.bin" || m_item->fileName == L"uc" || m_item->fileName.find(L'.') == std::wstring::npos) {
                        m_item->fileName = sanitized;
                        UpdateItemCategoryFromExt(m_item.get());
                    }
                }
            }

            // Check Google Drive virus warning in HTML
            wchar_t ctHeader[256] = { 0 };
            DWORD ctSize = sizeof(ctHeader);
            if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_TYPE, WINHTTP_HEADER_NAME_BY_INDEX, ctHeader, &ctSize, WINHTTP_NO_HEADER_INDEX)) {
                std::wstring ct = ctHeader;
                std::transform(ct.begin(), ct.end(), ct.begin(), ::towlower);
                if (ct.find(L"text/html") != std::wstring::npos && IsGoogleDriveUrl(fetchUrl)) {
                    std::vector<char> htmlBuf(16384, 0);
                    DWORD readHtml = 0;
                    WinHttpReadData(hRequest, htmlBuf.data(), 16383, &readHtml);
                    std::string htmlStr(htmlBuf.data(), readHtml);

                    std::string confirmToken = "";
                    size_t confPos = htmlStr.find("confirm=");
                    if (confPos != std::wstring::npos) {
                        size_t valStart = confPos + 8;
                        size_t valEnd = htmlStr.find_first_of("&\"' \r\n", valStart);
                        confirmToken = (valEnd == std::wstring::npos) ? htmlStr.substr(valStart) : htmlStr.substr(valStart, valEnd - valStart);
                    }
                    if (confirmToken.empty() && htmlStr.find("download_warning") != std::string::npos) {
                        confirmToken = "t";
                    }

                    if (!confirmToken.empty()) {
                        if (fetchUrl.find(L"confirm=") == std::wstring::npos) {
                            wchar_t wtok[128];
                            MultiByteToWideChar(CP_UTF8, 0, confirmToken.c_str(), -1, wtok, 128);
                            fetchUrl += (fetchUrl.find(L'?') == std::wstring::npos ? L"?" : L"&");
                            fetchUrl += L"confirm=";
                            fetchUrl += wtok;
                        }
                        WinHttpCloseHandle(hRequest);
                        WinHttpCloseHandle(hConnect);
                        continue;
                    }
                }
            }

            break;
        }

        if (statusCode != 200 && statusCode != 206) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            if (connIndex < (int)m_item->connections.size()) {
                wchar_t errBuf[64];
                swprintf_s(errBuf, L"HTTP %u", statusCode);
                m_item->connections[connIndex].status = errBuf;
            }
            return false;
        }

        if (statusCode == 200 && startByte > 0) {
            // Server did not honor Range request (returned HTTP 200 instead of 206 Partial Content).
            // It is sending the entire file from byte 0!
            // We MUST reset startByte and currentDownloaded to 0 so we don't append
            // duplicate data to the end of the existing file and grow the file indefinitely!
            startByte = 0;
        }

        uint64_t totalLength = 0;
        wchar_t contentRange[128] = { 0 };
        DWORD crSize = sizeof(contentRange);
        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_RANGE, WINHTTP_HEADER_NAME_BY_INDEX, contentRange, &crSize, WINHTTP_NO_HEADER_INDEX)) {
            wchar_t* slash = wcschr(contentRange, L'/');
            if (slash) {
                totalLength = _wcstoui64(slash + 1, NULL, 10);
            }
        }
        if (totalLength == 0) {
            wchar_t lenHeader[64] = { 0 };
            DWORD lenSize = sizeof(lenHeader);
            if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, lenHeader, &lenSize, WINHTTP_NO_HEADER_INDEX)) {
                totalLength = _wcstoui64(lenHeader, NULL, 10);
                if (statusCode == 206 && startByte > 0) {
                    totalLength += startByte;
                }
            }
        }
        if (totalLength == 0) {
            size_t clenPos = fetchUrl.find(L"clen=");
            if (clenPos != std::wstring::npos) {
                size_t valStart = clenPos + 5;
                size_t ampPos = fetchUrl.find(L'&', valStart);
                std::wstring clenStr = (ampPos != std::wstring::npos) ?
                    fetchUrl.substr(valStart, ampPos - valStart) : fetchUrl.substr(valStart);
                totalLength = _wcstoui64(clenStr.c_str(), NULL, 10);
            }
        }

        outTotalBytes = totalLength;
        if (connIndex < (int)m_item->connections.size()) {
            m_item->connections[connIndex].endByte = (totalLength > 0) ? (totalLength - 1) : 0;
            m_item->connections[connIndex].status = L"Receiving data...";
        }

        HANDLE hFile = CreateFileW(tempPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, (startByte > 0) ? OPEN_ALWAYS : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        // Enable NTFS sparse allocation so disk space is consumed only as data arrives
        DWORD dwReturned = 0;
        DeviceIoControl(hFile, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dwReturned, NULL);

        if (startByte > 0) {
            LARGE_INTEGER li;
            li.QuadPart = (LONGLONG)startByte;
            SetFilePointerEx(hFile, li, NULL, FILE_BEGIN);
        } else {
            LARGE_INTEGER li;
            li.QuadPart = 0;
            SetFilePointerEx(hFile, li, NULL, FILE_BEGIN);
            SetEndOfFile(hFile); // Truncate to 0 so stale or over-allocated bytes are removed
        }

        std::vector<char> buffer(256 * 1024); // 256 KB buffer for high speed throughput
        DWORD bytesRead = 0;
        uint64_t currentDownloaded = startByte;
        auto windowStartTime = std::chrono::steady_clock::now();
        uint64_t bytesReadInWindow = 0;

        auto lastSpeedTime = std::chrono::steady_clock::now();
        uint64_t lastSpeedDownloaded = currentDownloaded;

        while (!m_isPaused && !m_isCanceled) {
            // Speed limiting throttling
            if (m_item && m_item->speedLimitEnabled && m_item->speedLimitKBps > 0) {
                uint64_t limitBytes = (uint64_t)m_item->speedLimitKBps * 1024;
                if (limitBytes < 1024) limitBytes = 1024;

                auto now = std::chrono::steady_clock::now();
                auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - windowStartTime).count();

                if (elapsedMs < 1000 && bytesReadInWindow >= limitBytes) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000 - elapsedMs));
                    windowStartTime = std::chrono::steady_clock::now();
                    bytesReadInWindow = 0;
                } else if (elapsedMs >= 1000) {
                    windowStartTime = now;
                    bytesReadInWindow = 0;
                }
            }

            // Stop if we have completed known totalLength
            if (totalLength > 0 && currentDownloaded >= totalLength) {
                break;
            }

            DWORD toRead = (DWORD)buffer.size();
            if (totalLength > 0 && currentDownloaded + toRead > totalLength) {
                toRead = (DWORD)(totalLength - currentDownloaded);
            }
            if (toRead == 0) break;

            if (!WinHttpReadData(hRequest, buffer.data(), toRead, &bytesRead) || bytesRead == 0) {
                break;
            }
            DWORD bytesWritten = 0;
            WriteFile(hFile, buffer.data(), bytesRead, &bytesWritten, NULL);
            currentDownloaded += bytesRead;
            bytesReadInWindow += bytesRead;

            if (connIndex == 0) {
                m_item->downloadedBytes = currentDownloaded;
            } else {
                m_item->audioDownloadedBytes = currentDownloaded;
            }
            if (connIndex < (int)m_item->connections.size()) {
                m_item->connections[connIndex].downloadedBytes = currentDownloaded;
            }

            auto nowTime = std::chrono::steady_clock::now();
            double speedElapsed = std::chrono::duration<double>(nowTime - lastSpeedTime).count();
            if (speedElapsed >= 0.5) {
                double speed = (double)(currentDownloaded - lastSpeedDownloaded) / speedElapsed;
                m_item->speedBytesPerSec = (m_item->speedBytesPerSec * 0.7) + (speed * 0.3);
                if (m_item->speedBytesPerSec > 1.0 && m_item->totalBytes > currentDownloaded) {
                    m_item->timeLeftSec = (double)(m_item->totalBytes - currentDownloaded) / m_item->speedBytesPerSec;
                }
                lastSpeedDownloaded = currentDownloaded;
                lastSpeedTime = nowTime;
                if (m_callback) m_callback->OnProgress(m_item->id);
            }
        }

        CloseHandle(hFile);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        outTotalBytes = currentDownloaded;

        if (connIndex < (int)m_item->connections.size()) {
            m_item->connections[connIndex].status = (currentDownloaded > 0) ? L"Complete" : L"Error";
        }
        return currentDownloaded > 0;
    }

    void RunDashDownload() {
        m_item->status = DownloadStatus::Connecting;
        if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Connecting);

        wchar_t fullFilePath[MAX_PATH];
        PathCombineW(fullFilePath, m_item->savePath.c_str(), m_item->fileName.c_str());
        CreateDirectoryW(m_item->savePath.c_str(), NULL);

        std::wstring videoTemp = std::wstring(fullFilePath) + L".video.tmp";
        std::wstring audioTemp = std::wstring(fullFilePath) + L".audio.tmp";

        m_item->connections.clear();

        ConnectionInfo connV;
        connV.id = 1;
        connV.status = L"Connecting video stream...";
        m_item->connections.push_back(connV);

        ConnectionInfo connA;
        connA.id = 2;
        connA.status = L"Connecting audio stream...";
        m_item->connections.push_back(connA);

        std::atomic<bool> vDone(false), aDone(false);
        std::atomic<bool> vSuccess(false), aSuccess(false);

        std::thread tVideo([&]() {
            vSuccess = DownloadSingleStreamDirect(m_item->url, videoTemp, m_item->totalBytes, 0, L"Video");
            vDone = true;
        });

        std::thread tAudio([&]() {
            aSuccess = DownloadSingleStreamDirect(m_item->audioUrl, audioTemp, m_item->audioTotalBytes, 1, L"Audio");
            aDone = true;
        });

        m_item->status = DownloadStatus::Downloading;
        if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Downloading);

        auto lastTime = std::chrono::steady_clock::now();
        uint64_t lastDownloaded = 0;

        while (m_isRunning && !m_isPaused && !m_isCanceled && (!vDone || !aDone)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            uint64_t currentDownloaded = m_item->downloadedBytes + m_item->audioDownloadedBytes;
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - lastTime).count();
            if (elapsed >= 0.5) {
                double speed = (double)(currentDownloaded - lastDownloaded) / elapsed;
                m_item->speedBytesPerSec = (m_item->speedBytesPerSec * 0.7) + (speed * 0.3);
                uint64_t totalCombined = m_item->GetCombinedTotalBytes();
                if (m_item->speedBytesPerSec > 1.0 && totalCombined > currentDownloaded) {
                    m_item->timeLeftSec = (double)(totalCombined - currentDownloaded) / m_item->speedBytesPerSec;
                } else {
                    m_item->timeLeftSec = 0.0;
                }
                lastDownloaded = currentDownloaded;
                lastTime = now;
            }

            if (m_callback) m_callback->OnProgress(m_item->id);
        }

        if (tVideo.joinable()) tVideo.join();
        if (tAudio.joinable()) tAudio.join();

        if (m_isPaused) {
            m_item->status = DownloadStatus::Paused;
            if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Paused);
            m_isRunning = false;
            return;
        }
        if (m_isCanceled) {
            m_item->status = DownloadStatus::Canceled;
            if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Canceled);
            m_isRunning = false;
            return;
        }

        if (!vSuccess) {
            m_item->status = DownloadStatus::Error;
            if (m_callback) m_callback->OnFinished(m_item->id, false, L"Failed to download video stream");
            m_isRunning = false;
            return;
        }

        // Muxing Phase (DASH audio + video merge)
        m_item->isMerging = true;
        m_item->connections[0].status = L"Merging audio and video (DASH)...";
        m_item->connections[1].status = L"Merging audio and video (DASH)...";
        if (m_callback) {
            m_callback->OnProgress(m_item->id);
            m_callback->OnStatusChange(m_item->id, DownloadStatus::Downloading);
        }

        bool mergeOk = MergeDashStreams(videoTemp, audioTemp, fullFilePath);

        m_item->isMerging = false;
        if (mergeOk) {
            m_item->status = DownloadStatus::Complete;
            m_item->connections[0].status = L"Complete";
            m_item->connections[1].status = L"Complete";
            m_item->speedBytesPerSec = 0;
            m_item->timeLeftSec = 0;
            if (m_callback) {
                m_callback->OnProgress(m_item->id);
                m_callback->OnFinished(m_item->id, true, L"");
            }
        } else {
            m_item->status = DownloadStatus::Error;
            if (m_callback) m_callback->OnFinished(m_item->id, false, L"Failed to merge audio and video");
        }
        m_isRunning = false;
    }

    void RunYtDlpDownload(const std::wstring& videoUrl) {
        m_item->status = DownloadStatus::Downloading;
        if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Downloading);

        m_item->connections.clear();
        ConnectionInfo conn;
        conn.id = 1;
        conn.status = L"Connecting (DASH Video+Audio)...";
        m_item->connections.push_back(conn);

        wchar_t fullOut[MAX_PATH];
        std::wstring outTemplate;
        if (m_item->fileName.empty() || m_item->fileName == L"watch" || m_item->fileName == L"videoplayback" || m_item->fileName == L"videoplayback.mp4" || m_item->fileName == L"video.mp4" || m_item->fileName == L"download.bin") {
            wchar_t tpl[MAX_PATH];
            PathCombineW(tpl, m_item->savePath.c_str(), L"%(title)s.%(ext)s");
            outTemplate = tpl;
            PathCombineW(fullOut, m_item->savePath.c_str(), L"video.mp4");
        } else {
            std::wstring fn = m_item->fileName;
            std::wstring lower = fn;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
            if (lower.find(L".mp4") == std::wstring::npos && lower.find(L".mkv") == std::wstring::npos && lower.find(L".webm") == std::wstring::npos) {
                fn += L".mp4";
            }
            wchar_t tpl[MAX_PATH];
            PathCombineW(tpl, m_item->savePath.c_str(), fn.c_str());
            outTemplate = tpl;
            wcscpy_s(fullOut, tpl);
        }
        CreateDirectoryW(m_item->savePath.c_str(), NULL);

        std::wstring fmtFilter;
        if (m_item->qualityHeight > 0) {
            fmtFilter = L"bestvideo[height<=" + std::to_wstring(m_item->qualityHeight) + L"][ext=mp4]+bestaudio[ext=m4a]/bestvideo[height<=" + std::to_wstring(m_item->qualityHeight) + L"]+bestaudio/best[height<=" + std::to_wstring(m_item->qualityHeight) + L"]/best";
        } else {
            fmtFilter = L"bestvideo[ext=mp4]+bestaudio[ext=m4a]/bestvideo+bestaudio/best";
        }

        std::wstring ytdlp = LocateYtDlp();
        std::wstring toolPrefix = !ytdlp.empty() ? (L"\"" + ytdlp + L"\"") : L"yt-dlp";
        std::wstring extraHdrs;
        if (!m_item->userAgent.empty()) {
            extraHdrs += L" --user-agent \"" + m_item->userAgent + L"\"";
        }
        extraHdrs += L" --referer \"https://www.youtube.com/\"";

        std::wstring ffmpeg = LocateFfmpeg();
        std::wstring ffmpegDir;
        if (ffmpeg.find(L'\\') != std::wstring::npos) {
            wchar_t fDir[MAX_PATH];
            wcscpy_s(fDir, ffmpeg.c_str());
            PathRemoveFileSpecW(fDir);
            ffmpegDir = fDir;
        }
        std::wstring ffmpegArg = !ffmpegDir.empty() ? (L" --ffmpeg-location \"" + ffmpegDir + L"\"") : L"";

        std::wstring cmd = toolPrefix + L" --newline --no-colors --windows-filenames --no-playlist" + ffmpegArg +
            L" -f \"" + fmtFilter + L"\" --merge-output-format mp4" + extraHdrs + L" -o \"" +
            outTemplate + L"\" \"" + videoUrl + L"\"";

        SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
        HANDLE hReadPipe, hWritePipe;
        if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
            m_item->status = DownloadStatus::Error;
            if (m_callback) m_callback->OnFinished(m_item->id, false, L"Failed to create execution pipe");
            return;
        }
        SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOW si = { sizeof(si) };
        si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        si.wShowWindow = SW_HIDE;
        si.hStdOutput = hWritePipe;
        si.hStdError = hWritePipe;
        PROCESS_INFORMATION pi = { 0 };

        std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
        cmdBuf.push_back(0);

        if (!CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            CloseHandle(hWritePipe);
            CloseHandle(hReadPipe);
            m_item->status = DownloadStatus::Error;
            if (m_callback) m_callback->OnFinished(m_item->id, false, L"Failed to launch yt-dlp downloader");
            return;
        }
        CloseHandle(hWritePipe);
        m_hChildProcess = pi.hProcess;

        char buffer[1024];
        DWORD bytesRead = 0;
        std::string lineBuf;

        while (!m_isPaused && !m_isCanceled && ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = 0;
            lineBuf += buffer;

            size_t newline;
            while ((newline = lineBuf.find('\n')) != std::string::npos) {
                std::string line = lineBuf.substr(0, newline);
                lineBuf.erase(0, newline + 1);

                if (line.find("[download] Destination: ") != std::string::npos) {
                    std::string dest = line.substr(line.find("[download] Destination: ") + 24);
                    while (!dest.empty() && (dest.back() == '\r' || dest.back() == ' ')) dest.pop_back();
                    int wlen = MultiByteToWideChar(CP_UTF8, 0, dest.c_str(), -1, NULL, 0);
                    if (wlen > 0) {
                        std::wstring wdest(wlen, 0);
                        MultiByteToWideChar(CP_UTF8, 0, dest.c_str(), -1, &wdest[0], wlen);
                        if (!wdest.empty() && wdest.back() == L'\0') wdest.pop_back();
                        wcscpy_s(fullOut, wdest.c_str());
                        wchar_t* pfn = PathFindFileNameW(fullOut);
                        if (pfn && wcslen(pfn) > 0) m_item->fileName = pfn;
                    }
                } else if (line.find("[Merger] Merging formats into \"") != std::string::npos) {
                    size_t q1 = line.find('"');
                    size_t q2 = line.rfind('"');
                    if (q1 != std::string::npos && q2 != std::string::npos && q2 > q1) {
                        std::string dest = line.substr(q1 + 1, q2 - q1 - 1);
                        int wlen = MultiByteToWideChar(CP_UTF8, 0, dest.c_str(), -1, NULL, 0);
                        if (wlen > 0) {
                            std::wstring wdest(wlen, 0);
                            MultiByteToWideChar(CP_UTF8, 0, dest.c_str(), -1, &wdest[0], wlen);
                            if (!wdest.empty() && wdest.back() == L'\0') wdest.pop_back();
                            wcscpy_s(fullOut, wdest.c_str());
                            wchar_t* pfn = PathFindFileNameW(fullOut);
                            if (pfn && wcslen(pfn) > 0) m_item->fileName = pfn;
                        }
                    }
                }

                if (line.find("[download]") != std::string::npos) {
                    size_t pctPos = line.find('%');
                    if (pctPos != std::string::npos) {
                        size_t start = line.rfind(' ', pctPos - 1);
                        if (start != std::string::npos) {
                            double pct = atof(line.c_str() + start + 1);
                            size_t ofPos = line.find("of", pctPos);
                            if (ofPos != std::string::npos) {
                                size_t atPos = line.find("at", ofPos);
                                if (atPos != std::string::npos) {
                                    std::string szToken = line.substr(ofPos + 2, atPos - (ofPos + 2));
                                    while (!szToken.empty() && (szToken.front() == ' ' || szToken.front() == '~')) szToken.erase(szToken.begin());
                                    while (!szToken.empty() && szToken.back() == ' ') szToken.pop_back();
                                    double szVal = atof(szToken.c_str());
                                    uint64_t mult = 1;
                                    if (szToken.find("GiB") != std::string::npos || szToken.find("GB") != std::string::npos) mult = 1024ULL * 1024ULL * 1024ULL;
                                    else if (szToken.find("MiB") != std::string::npos || szToken.find("MB") != std::string::npos) mult = 1024ULL * 1024ULL;
                                    else if (szToken.find("KiB") != std::string::npos || szToken.find("KB") != std::string::npos) mult = 1024ULL;
                                    if (szVal > 0) {
                                        m_item->totalBytes = (uint64_t)(szVal * mult);
                                        m_item->downloadedBytes = (uint64_t)((pct / 100.0) * m_item->totalBytes);
                                    }
                                }
                            }
                        }
                    }
                    size_t atPos = line.find("at ");
                    if (atPos != std::string::npos) {
                        size_t etaPos = line.find("ETA", atPos);
                        std::string spdToken = (etaPos != std::string::npos) ?
                            line.substr(atPos + 3, etaPos - (atPos + 3)) : line.substr(atPos + 3);
                        while (!spdToken.empty() && spdToken.front() == ' ') spdToken.erase(spdToken.begin());
                        while (!spdToken.empty() && spdToken.back() == ' ') spdToken.pop_back();
                        double spdVal = atof(spdToken.c_str());
                        uint64_t mult = 1;
                        if (spdToken.find("GiB") != std::string::npos || spdToken.find("GB") != std::string::npos) mult = 1024ULL * 1024ULL * 1024ULL;
                        else if (spdToken.find("MiB") != std::string::npos || spdToken.find("MB") != std::string::npos) mult = 1024ULL * 1024ULL;
                        else if (spdToken.find("KiB") != std::string::npos || spdToken.find("KB") != std::string::npos) mult = 1024ULL;
                        if (spdVal > 0) m_item->speedBytesPerSec = spdVal * mult;
                    }
                    size_t etaPos = line.find("ETA ");
                    if (etaPos != std::string::npos) {
                        std::string etaStr = line.substr(etaPos + 4);
                        int m = 0, s = 0;
                        if (sscanf_s(etaStr.c_str(), "%d:%d", &m, &s) == 2) {
                            m_item->timeLeftSec = m * 60 + s;
                        }
                    }
                    if (!m_item->connections.empty()) {
                        m_item->connections[0].downloadedBytes = m_item->downloadedBytes;
                        m_item->connections[0].endByte = m_item->totalBytes > 0 ? m_item->totalBytes - 1 : 0;
                        m_item->connections[0].status = L"Receiving data...";
                    }
                } else if (line.find("[Merger]") != std::string::npos || line.find("Merging") != std::string::npos) {
                    m_item->isMerging = true;
                    if (!m_item->connections.empty()) {
                        m_item->connections[0].status = L"Merging audio and video (DASH)...";
                    }
                }
                if (m_callback) m_callback->OnProgress(m_item->id);
            }
        }
        CloseHandle(hReadPipe);
        DWORD exitCode = 0;
        if (pi.hProcess) {
            WaitForSingleObject(pi.hProcess, 1000);
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
        }
        if (pi.hThread) {
            CloseHandle(pi.hThread);
        }
        m_hChildProcess = NULL;

        if (m_isPaused) {
            m_item->status = DownloadStatus::Paused;
            m_item->speedBytesPerSec = 0;
            m_item->timeLeftSec = 0;
            if (!m_item->connections.empty()) m_item->connections[0].status = L"Paused";
            if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Paused);
            m_isRunning = false;
            return;
        }
        if (m_isCanceled) {
            m_item->status = DownloadStatus::Canceled;
            m_item->speedBytesPerSec = 0;
            m_item->timeLeftSec = 0;
            if (!m_item->connections.empty()) m_item->connections[0].status = L"Canceled";
            if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Canceled);
            m_isRunning = false;
            return;
        }

        m_item->isMerging = false;
        if (exitCode == 0) {
            HANDLE hChk = CreateFileW(fullOut, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hChk != INVALID_HANDLE_VALUE) {
                LARGE_INTEGER sz;
                GetFileSizeEx(hChk, &sz);
                CloseHandle(hChk);
                m_item->totalBytes = sz.QuadPart;
                m_item->downloadedBytes = sz.QuadPart;
            } else {
                WIN32_FIND_DATAW ffd;
                wchar_t searchPattern[MAX_PATH];
                PathCombineW(searchPattern, m_item->savePath.c_str(), L"*.mp4");
                HANDLE hFind = FindFirstFileW(searchPattern, &ffd);
                if (hFind != INVALID_HANDLE_VALUE) {
                    FILETIME latestTime = { 0, 0 };
                    std::wstring bestFile;
                    uint64_t bestSize = 0;
                    do {
                        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                            if (CompareFileTime(&ffd.ftLastWriteTime, &latestTime) > 0) {
                                latestTime = ffd.ftLastWriteTime;
                                bestFile = ffd.cFileName;
                                ULARGE_INTEGER ul;
                                ul.LowPart = ffd.nFileSizeLow;
                                ul.HighPart = ffd.nFileSizeHigh;
                                bestSize = ul.QuadPart;
                            }
                        }
                    } while (FindNextFileW(hFind, &ffd));
                    FindClose(hFind);
                    if (!bestFile.empty() && bestSize > 1024) {
                        m_item->fileName = bestFile;
                        PathCombineW(fullOut, m_item->savePath.c_str(), bestFile.c_str());
                        m_item->totalBytes = bestSize;
                        m_item->downloadedBytes = bestSize;
                    }
                }
            }
            m_item->status = DownloadStatus::Complete;
            if (!m_item->connections.empty()) {
                m_item->connections[0].status = L"Complete";
            }
            m_item->speedBytesPerSec = 0;
            m_item->timeLeftSec = 0;
            if (m_callback) {
                m_callback->OnProgress(m_item->id);
                m_callback->OnFinished(m_item->id, true, L"");
            }
        } else {
            m_item->status = DownloadStatus::Error;
            if (m_callback) m_callback->OnFinished(m_item->id, false, L"Video download failed");
        }
    }

    void Run() {
        if (!m_item) return;

        if (IsGoogleVideoUrl(m_item->url)) {
            m_item->url = SanitizeGoogleVideoUrl(m_item->url);
        }
        if (IsGoogleVideoUrl(m_item->audioUrl)) {
            m_item->audioUrl = SanitizeGoogleVideoUrl(m_item->audioUrl);
        }

        std::wstring youtubePage;
        if (IsYouTubeWatchUrl(m_item->url)) {
            youtubePage = m_item->url;
        } else if (IsYouTubeWatchUrl(m_item->referer)) {
            youtubePage = m_item->referer;
        } else if ((IsGoogleVideoUrl(m_item->url) || m_item->isDash) && !m_item->referer.empty() && m_item->referer.find(L"youtube.com") != std::wstring::npos) {
            youtubePage = m_item->referer;
        }

        // YouTube page URL (or sniffed googlevideo tied to a watch page) must go through yt-dlp.
        // Sniffed UMP/SABR segments are ~30KB init packets, not full videos.
        if (!youtubePage.empty()) {
            RunYtDlpDownload(youtubePage);
            return;
        }

        // 1. DASH stream: Audio + Video separate streams (Facebook, etc.)
        if (m_item->isDash && !m_item->audioUrl.empty()) {
            RunDashDownload();
            return;
        }

        // 2. Direct googlevideo without a watch-page referer
        if (IsGoogleVideoUrl(m_item->url)) {
            if (!m_item->audioUrl.empty()) {
                RunDashDownload();
                return;
            }
        }

        // 3. Other web video pages
        if (IsWebVideoPageUrl(m_item->url)) {
            RunYtDlpDownload(m_item->url);
            return;
        }

        if (IsGoogleDriveUrl(m_item->url)) {
            m_item->url = NormalizeGoogleDriveUrl(m_item->url);
        }

        m_item->status = DownloadStatus::Connecting;
        if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Connecting);

        std::wstring actualUA = !m_item->userAgent.empty() ? m_item->userAgent : L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/155.0.0.0 Safari/537.36";

        HINTERNET hSession = WinHttpOpen(actualUA.c_str(),
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

        if (!hSession) {
            m_item->status = DownloadStatus::Error;
            if (m_callback) m_callback->OnFinished(m_item->id, false, L"Failed to initialize WinHTTP");
            m_isRunning = false;
            return;
        }

        DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        WinHttpSetOption(hSession, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

        DWORD secureProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
        WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secureProtocols, sizeof(secureProtocols));
        WinHttpSetTimeouts(hSession, 15000, 20000, 30000, 30000);

        DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                         SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;

        std::wstring fetchUrl = m_item->url;
        std::wstring finalHost = L"";
        std::wstring finalPath = L"";
        INTERNET_PORT finalPort = 80;
        DWORD finalFlags = 0;
        uint64_t totalLength = 0;
        bool supportsRanges = false;

        int redirectHops = 0;
        const int maxHops = 10;
        bool probeFinished = false;
        bool triedUAFallback = false;
        bool sendRangeHeader = true;

        while (redirectHops < maxHops && !m_isCanceled) {
            redirectHops++;
            URL_COMPONENTS urlComp = { 0 };
            urlComp.dwStructSize = sizeof(urlComp);
            std::vector<wchar_t> hostName(2048, 0);
            std::vector<wchar_t> urlPath(32768, 0);
            urlComp.lpszHostName = hostName.data();
            urlComp.dwHostNameLength = (DWORD)hostName.size();
            urlComp.lpszUrlPath = urlPath.data();
            urlComp.dwUrlPathLength = (DWORD)urlPath.size();

            if (!WinHttpCrackUrl(fetchUrl.c_str(), (DWORD)fetchUrl.length(), 0, &urlComp)) {
                break;
            }

            finalHost.assign(hostName.begin(), hostName.begin() + urlComp.dwHostNameLength);
            finalPath.assign(urlPath.begin(), urlPath.begin() + urlComp.dwUrlPathLength);
            finalPort = urlComp.nPort;
            finalFlags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;

            HINTERNET hConnect = WinHttpConnect(hSession, finalHost.c_str(), finalPort, 0);
            if (!hConnect) break;

            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", finalPath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, finalFlags);
            if (!hRequest) {
                WinHttpCloseHandle(hConnect);
                break;
            }

            DWORD http2Flag = WINHTTP_PROTOCOL_FLAG_HTTP2;
            WinHttpSetOption(hRequest, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, &http2Flag, sizeof(http2Flag));
            WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
            WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

            std::wstring browserHdrs = L"User-Agent: " + actualUA + L"\r\n"
                L"Accept: */*\r\n"
                L"Accept-Language: en-US,en;q=0.9\r\n";
            if (!m_item->referer.empty()) {
                browserHdrs += L"Referer: " + m_item->referer + L"\r\n";
            } else if (IsGoogleVideoUrl(fetchUrl)) {
                browserHdrs += L"Referer: https://www.youtube.com/\r\n";
            }
            if (IsGoogleVideoUrl(fetchUrl)) {
                browserHdrs += L"Origin: https://www.youtube.com\r\n"
                               L"Sec-Fetch-Dest: video\r\n"
                               L"Sec-Fetch-Mode: cors\r\n"
                               L"Sec-Fetch-Site: cross-site\r\n";
            }
            if (!m_item->cookies.empty()) {
                browserHdrs += L"Cookie: " + m_item->cookies + L"\r\n";
            }
            if (!IsGoogleVideoUrl(fetchUrl) && sendRangeHeader) {
                browserHdrs += L"Range: bytes=0-0\r\n";
            }

            WinHttpAddRequestHeaders(hRequest, browserHdrs.c_str(), (DWORD)browserHdrs.length(), WINHTTP_ADDREQ_FLAG_ADD);

            if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
                !WinHttpReceiveResponse(hRequest, NULL)) {
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                break;
            }

            DWORD statusCode = 0;
            DWORD scSize = sizeof(statusCode);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &scSize, WINHTTP_NO_HEADER_INDEX);

            // Bypass Cloudflare/WAF block when browser UA is blocked on non-browser TLS
            if ((statusCode == 403 || statusCode == 406) && !triedUAFallback) {
                triedUAFallback = true;
                actualUA = L"curl/8.4.0";
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                hSession = WinHttpOpen(actualUA.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
                if (!hSession) break;
                WinHttpSetOption(hSession, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));
                WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secureProtocols, sizeof(secureProtocols));
                WinHttpSetTimeouts(hSession, 15000, 20000, 30000, 30000);
                continue;
            }

            // If server rejects Range: bytes=0-0 with 416 or 400, retry probe without Range header
            if ((statusCode == 416 || statusCode == 400) && sendRangeHeader) {
                sendRangeHeader = false;
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                continue;
            }

            if (statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 307 || statusCode == 308) {
                std::vector<wchar_t> locHeader(8192, 0);
                DWORD locSize = (DWORD)(locHeader.size() * sizeof(wchar_t));
                if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX, locHeader.data(), &locSize, WINHTTP_NO_HEADER_INDEX)) {
                    std::vector<wchar_t> combBuf(8192, 0);
                    DWORD combLen = (DWORD)combBuf.size();
                    if (SUCCEEDED(UrlCombineW(fetchUrl.c_str(), locHeader.data(), combBuf.data(), &combLen, 0))) {
                        fetchUrl = combBuf.data();
                    } else {
                        fetchUrl = locHeader.data();
                    }
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(hConnect);
                    continue;
                }
            }

            // Extract real filename from Content-Disposition
            wchar_t cdHeader[1024] = { 0 };
            DWORD cdSize = sizeof(cdHeader);
            if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_DISPOSITION, WINHTTP_HEADER_NAME_BY_INDEX, cdHeader, &cdSize, WINHTTP_NO_HEADER_INDEX)) {
                std::wstring parsed = ExtractFilenameFromContentDisposition(cdHeader);
                if (!parsed.empty()) {
                    std::wstring sanitized = SanitizeFilename(parsed);
                    if (m_item->fileName.empty() || m_item->fileName == L"download.bin" || m_item->fileName == L"uc" || m_item->fileName.find(L'.') == std::wstring::npos) {
                        m_item->fileName = sanitized;
                        UpdateItemCategoryFromExt(m_item.get());
                    }
                }
            }

            // Check for Google Drive virus warning in HTML
            wchar_t ctHeader[256] = { 0 };
            DWORD ctSize = sizeof(ctHeader);
            if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_TYPE, WINHTTP_HEADER_NAME_BY_INDEX, ctHeader, &ctSize, WINHTTP_NO_HEADER_INDEX)) {
                std::wstring ct = ctHeader;
                std::transform(ct.begin(), ct.end(), ct.begin(), ::towlower);
                if (ct.find(L"text/html") != std::wstring::npos && IsGoogleDriveUrl(fetchUrl)) {
                    std::vector<char> htmlBuf(16384, 0);
                    DWORD readHtml = 0;
                    WinHttpReadData(hRequest, htmlBuf.data(), 16383, &readHtml);
                    std::string htmlStr(htmlBuf.data(), readHtml);

                    std::string confirmToken = "";
                    size_t confPos = htmlStr.find("confirm=");
                    if (confPos != std::wstring::npos) {
                        size_t valStart = confPos + 8;
                        size_t valEnd = htmlStr.find_first_of("&\"' \r\n", valStart);
                        confirmToken = (valEnd == std::wstring::npos) ? htmlStr.substr(valStart) : htmlStr.substr(valStart, valEnd - valStart);
                    }
                    if (confirmToken.empty() && htmlStr.find("download_warning") != std::string::npos) {
                        confirmToken = "t";
                    }

                    if (!confirmToken.empty()) {
                        if (fetchUrl.find(L"confirm=") == std::wstring::npos) {
                            wchar_t wtok[128];
                            MultiByteToWideChar(CP_UTF8, 0, confirmToken.c_str(), -1, wtok, 128);
                            fetchUrl += (fetchUrl.find(L'?') == std::wstring::npos ? L"?" : L"&");
                            fetchUrl += L"confirm=";
                            fetchUrl += wtok;
                        }
                        WinHttpCloseHandle(hRequest);
                        WinHttpCloseHandle(hConnect);
                        continue;
                    }
                }
            }

            if (statusCode == 206) {
                supportsRanges = true;
                wchar_t contentRange[128] = { 0 };
                DWORD crSize = sizeof(contentRange);
                if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_RANGE, WINHTTP_HEADER_NAME_BY_INDEX, contentRange, &crSize, WINHTTP_NO_HEADER_INDEX)) {
                    wchar_t* slash = wcschr(contentRange, L'/');
                    if (slash) {
                        totalLength = _wcstoui64(slash + 1, NULL, 10);
                    }
                }
                probeFinished = true;
            } else if (statusCode == 200) {
                wchar_t lenHeader[64] = { 0 };
                DWORD lenSize = sizeof(lenHeader);
                if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, lenHeader, &lenSize, WINHTTP_NO_HEADER_INDEX)) {
                    totalLength = _wcstoui64(lenHeader, NULL, 10);
                }
                wchar_t rangeHeader[64] = { 0 };
                DWORD rangeSize = sizeof(rangeHeader);
                if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_ACCEPT_RANGES, WINHTTP_HEADER_NAME_BY_INDEX, rangeHeader, &rangeSize, WINHTTP_NO_HEADER_INDEX)) {
                    if (_wcsicmp(rangeHeader, L"bytes") == 0) {
                        supportsRanges = true;
                    }
                }
                probeFinished = true;
            }

            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            break;
        }

        WinHttpCloseHandle(hSession);

        // YouTube clen parameter fallback
        if (totalLength == 0) {
            size_t clenPos = fetchUrl.find(L"clen=");
            if (clenPos != std::wstring::npos) {
                size_t valStart = clenPos + 5;
                size_t ampPos = fetchUrl.find(L'&', valStart);
                std::wstring clenStr = (ampPos != std::wstring::npos) ?
                    fetchUrl.substr(valStart, ampPos - valStart) : fetchUrl.substr(valStart);
                totalLength = _wcstoui64(clenStr.c_str(), NULL, 10);
                if (totalLength > 0) supportsRanges = true;
            }
        }

        m_item->url = fetchUrl;
        m_item->totalBytes = totalLength;
        m_item->resumeCapable = supportsRanges;

        // Ensure target directory exists
        wchar_t fullFilePath[MAX_PATH];
        PathCombineW(fullFilePath, m_item->savePath.c_str(), m_item->fileName.c_str());
        CreateDirectoryW(m_item->savePath.c_str(), NULL);

        // Determine connections count
        // Google Drive, YouTube, Cloud/ChatGPT, and servers without ranges MUST use 1 reliable sequential stream
        int numConns = 1;
        if (!IsCloudOrProtectedUrl(fetchUrl) && !IsGoogleVideoUrl(fetchUrl) && supportsRanges && totalLength > 1024 * 1024) {
            numConns = m_item->numConnections > 0 ? m_item->numConnections : 8;
        }

        // If single stream is selected, use DownloadSingleStreamDirect directly for 100% rock-solid reliability
        if (numConns <= 1) {
            m_item->connections.clear();
            ConnectionInfo conn;
            conn.id = 1;
            conn.startByte = 0;
            conn.endByte = (totalLength > 0) ? (totalLength - 1) : 0;
            conn.downloadedBytes = 0;
            conn.status = L"Connecting...";
            m_item->connections.push_back(conn);

            m_item->status = DownloadStatus::Downloading;
            if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Downloading);

            uint64_t finalDownloaded = 0;
            bool ok = DownloadSingleStreamDirect(fetchUrl, fullFilePath, finalDownloaded, 0, L"Sequential");

            if (m_isPaused) {
                m_item->status = DownloadStatus::Paused;
                if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Paused);
            } else if (m_isCanceled) {
                m_item->status = DownloadStatus::Canceled;
                if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Canceled);
            } else if (ok && (finalDownloaded > 0 || m_item->downloadedBytes > 0)) {
                if (finalDownloaded > 0) m_item->downloadedBytes = finalDownloaded;
                if (m_item->totalBytes == 0) m_item->totalBytes = m_item->downloadedBytes;
                m_item->status = DownloadStatus::Complete;
                m_item->speedBytesPerSec = 0;
                m_item->timeLeftSec = 0;
                if (m_callback) {
                    m_callback->OnProgress(m_item->id);
                    m_callback->OnFinished(m_item->id, true, L"");
                }
            } else {
                m_item->status = DownloadStatus::Error;
                m_item->speedBytesPerSec = 0;
                m_item->timeLeftSec = 0;
                if (m_callback) {
                    m_callback->OnProgress(m_item->id);
                    m_callback->OnFinished(m_item->id, false, L"Connection error or server rejected transfer");
                }
            }

            m_isRunning = false;
            return;
        }

        // Multi-connection accelerated download
        bool isResuming = (m_item->resumeCapable && !m_item->connections.empty() && m_item->downloadedBytes > 0);
        if (!isResuming) {
            m_item->connections.clear();
            uint64_t chunkSize = (numConns > 1 && totalLength > 0) ? (totalLength / numConns) : totalLength;

            for (int i = 0; i < numConns; i++) {
                ConnectionInfo conn;
                conn.id = i + 1;
                conn.startByte = i * chunkSize;
                conn.endByte = (i == numConns - 1 && totalLength > 0) ? (totalLength - 1) : ((i + 1) * chunkSize - 1);
                conn.downloadedBytes = 0;
                conn.status = L"Connecting...";
                m_item->connections.push_back(conn);
            }

            HANDLE hFile = CreateFileW(fullFilePath, GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

            if (hFile != INVALID_HANDLE_VALUE) {
                // Enable Sparse File on NTFS so that preallocating the logical file size
                // does NOT physically allocate disk sectors or zero-fill on disk!
                DWORD dwReturned = 0;
                DeviceIoControl(hFile, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dwReturned, NULL);

                if (totalLength > 200 * 1024) {
                    LARGE_INTEGER li;
                    li.QuadPart = totalLength;
                    SetFilePointerEx(hFile, li, NULL, FILE_BEGIN);
                    SetEndOfFile(hFile);
                }
                CloseHandle(hFile);
            }
        } else {
            for (auto& conn : m_item->connections) {
                uint64_t segLen = (conn.endByte >= conn.startByte) ? (conn.endByte - conn.startByte + 1) : 0;
                if (segLen > 0 && conn.downloadedBytes >= segLen) {
                    conn.status = L"Complete";
                } else {
                    conn.status = L"Connecting...";
                }
            }
        }

        m_item->status = DownloadStatus::Downloading;
        if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Downloading);

        // Spawn connection worker threads ONLY for incomplete chunks!
        m_workerThreads.clear();
        for (int i = 0; i < (int)m_item->connections.size(); i++) {
            uint64_t segLen = (m_item->connections[i].endByte >= m_item->connections[i].startByte) ?
                (m_item->connections[i].endByte - m_item->connections[i].startByte + 1) : 0;
            if (segLen > 0 && m_item->connections[i].downloadedBytes >= segLen) {
                m_item->connections[i].status = L"Complete";
                continue; // Chunk is already 100% finished, skip!
            }
            m_workerThreads.emplace_back(&DownloadTask::WorkerThread, this, i, finalHost, finalPort, finalPath, finalFlags, fullFilePath);
        }

        // Speed & progress monitor loop with Dynamic Segment Splitting
        auto lastTime = std::chrono::steady_clock::now();
        auto lastSplitTime = std::chrono::steady_clock::now();
        uint64_t lastDownloaded = 0;

        while (m_isRunning && !m_isPaused && !m_isCanceled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            uint64_t currentDownloaded = 0;
            bool allComplete = true;
            bool anyWorkerRunning = false;

            for (size_t i = 0; i < m_item->connections.size(); i++) {
                currentDownloaded += m_item->connections[i].downloadedBytes;
                uint64_t segLen = (m_item->connections[i].endByte >= m_item->connections[i].startByte) ?
                    (m_item->connections[i].endByte - m_item->connections[i].startByte + 1) : 0;

                bool isChunkDone = (segLen > 0 && m_item->connections[i].downloadedBytes >= segLen) || (m_item->connections[i].status == L"Complete");
                if (!isChunkDone) {
                    allComplete = false;
                    const std::wstring& st = m_item->connections[i].status;
                    if (st == L"Connecting..." || st == L"Send GET..." || st == L"Receiving data..." ||
                        st.find(L"Retrying") != std::wstring::npos || st.find(L"Split") != std::wstring::npos || st.find(L"Reconnecting") != std::wstring::npos) {
                        anyWorkerRunning = true;
                    }
                }
            }

            m_item->downloadedBytes = currentDownloaded;

            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - lastTime).count();
            if (elapsed >= 0.5) {
                double speed = (double)(currentDownloaded - lastDownloaded) / elapsed;
                m_item->speedBytesPerSec = (m_item->speedBytesPerSec * 0.7) + (speed * 0.3); // Smoothed
                if (m_item->speedBytesPerSec > 1.0 && m_item->totalBytes > currentDownloaded) {
                    m_item->timeLeftSec = (double)(m_item->totalBytes - currentDownloaded) / m_item->speedBytesPerSec;
                } else {
                    m_item->timeLeftSec = 0.0;
                }
                lastDownloaded = currentDownloaded;
                lastTime = now;
            }

            if (m_callback) m_callback->OnProgress(m_item->id);

            // If all chunks completed, break
            if (allComplete || (totalLength > 0 && currentDownloaded >= totalLength)) {
                if (m_item->totalBytes == 0 && currentDownloaded > 0) {
                    m_item->totalBytes = currentDownloaded;
                }
                break;
            }

            // Dynamic Segment Splitting:
            // When one connection finishes its chunk and another connection has >= 4MB remaining,
            // split the largest active connection in half so idle connection helps download it!
            double sinceLastSplit = std::chrono::duration<double>(now - lastSplitTime).count();
            if (sinceLastSplit >= 1.5 && supportsRanges && totalLength > 0 && !m_isPaused && !m_isCanceled) {
                int idleIdx = -1;
                for (size_t i = 0; i < m_item->connections.size(); i++) {
                    uint64_t segLen = (m_item->connections[i].endByte >= m_item->connections[i].startByte) ?
                        (m_item->connections[i].endByte - m_item->connections[i].startByte + 1) : 0;
                    if (m_item->connections[i].status == L"Complete" || (segLen > 0 && m_item->connections[i].downloadedBytes >= segLen)) {
                        idleIdx = (int)i;
                        break;
                    }
                }

                if (idleIdx != -1) {
                    int largestIdx = -1;
                    uint64_t maxRemaining = 0;
                    for (size_t i = 0; i < m_item->connections.size(); i++) {
                        if ((int)i == idleIdx) continue;
                        if (m_item->connections[i].status == L"Receiving data...") {
                            uint64_t curPos = m_item->connections[i].startByte + m_item->connections[i].downloadedBytes;
                            if (m_item->connections[i].endByte > curPos) {
                                uint64_t rem = m_item->connections[i].endByte - curPos + 1;
                                if (rem > maxRemaining) {
                                    maxRemaining = rem;
                                    largestIdx = (int)i;
                                }
                            }
                        }
                    }

                    // Only split if at least 4 MB remains to be downloaded in that chunk
                    if (largestIdx != -1 && maxRemaining >= 4ULL * 1024ULL * 1024ULL) {
                        uint64_t curPos = m_item->connections[largestIdx].startByte + m_item->connections[largestIdx].downloadedBytes;
                        uint64_t half = maxRemaining / 2;
                        uint64_t splitPos = curPos + half;
                        uint64_t origEnd = m_item->connections[largestIdx].endByte;

                        // Shorten largest worker's end
                        m_item->connections[largestIdx].endByte = splitPos - 1;

                        // Assign idle worker to second half
                        m_item->connections[idleIdx].startByte = splitPos;
                        m_item->connections[idleIdx].endByte = origEnd;
                        m_item->connections[idleIdx].downloadedBytes = 0;
                        m_item->connections[idleIdx].status = L"Connecting (Split)...";

                        lastSplitTime = now;
                        anyWorkerRunning = true;

                        m_workerThreads.emplace_back(&DownloadTask::WorkerThread, this, idleIdx, finalHost, finalPort, finalPath, finalFlags, fullFilePath);
                    }
                }
            }

            // If all worker threads have finished (or failed)
            if (!anyWorkerRunning) {
                break;
            }
        }

        // Wait for workers to finish
        for (auto& t : m_workerThreads) {
            if (t.joinable()) t.join();
        }
        m_workerThreads.clear();

        // If some chunks remain incomplete (e.g. server concurrency limit like dlproxy),
        // try finishing incomplete chunks sequentially one-by-one!
        if (!m_isPaused && !m_isCanceled && (totalLength == 0 || m_item->downloadedBytes < totalLength)) {
            for (size_t i = 0; i < m_item->connections.size() && !m_isPaused && !m_isCanceled; i++) {
                auto& conn = m_item->connections[i];
                uint64_t segLen = (conn.endByte >= conn.startByte) ? (conn.endByte - conn.startByte + 1) : 0;
                if (segLen > 0 && conn.downloadedBytes < segLen) {
                    conn.status = L"Connecting (Sequential retry)...";
                    if (m_callback) m_callback->OnProgress(m_item->id);
                    WorkerThread((int)i, finalHost, finalPort, finalPath, finalFlags, fullFilePath);

                    uint64_t curDl = 0;
                    for (const auto& c : m_item->connections) curDl += c.downloadedBytes;
                    m_item->downloadedBytes = curDl;
                    if (m_callback) m_callback->OnProgress(m_item->id);
                }
            }
        }

        // Final tally
        uint64_t finalDownloaded = 0;
        bool allSegmentsComplete = true;
        for (const auto& conn : m_item->connections) {
            finalDownloaded += conn.downloadedBytes;
            uint64_t segLen = (conn.endByte >= conn.startByte) ? (conn.endByte - conn.startByte + 1) : 0;
            if (segLen > 0 && conn.downloadedBytes < segLen) {
                allSegmentsComplete = false;
            }
        }
        m_item->downloadedBytes = finalDownloaded;

        if (m_isPaused) {
            m_item->status = DownloadStatus::Paused;
            if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Paused);
        } else if (m_isCanceled) {
            m_item->status = DownloadStatus::Canceled;
            if (m_callback) m_callback->OnStatusChange(m_item->id, DownloadStatus::Canceled);
        } else if (allSegmentsComplete || (totalLength > 0 && finalDownloaded >= totalLength)) {
            m_item->status = DownloadStatus::Complete;
            m_item->speedBytesPerSec = 0;
            m_item->timeLeftSec = 0;
            if (m_callback) {
                m_callback->OnProgress(m_item->id);
                m_callback->OnFinished(m_item->id, true, L"");
            }
        } else {
            m_item->status = DownloadStatus::Error;
            m_item->speedBytesPerSec = 0;
            m_item->timeLeftSec = 0;
            if (m_callback) {
                m_callback->OnProgress(m_item->id);
                m_callback->OnFinished(m_item->id, false, L"Transfer incomplete or server connection closed (Right-click to refresh address or resume)");
            }
        }

        m_isRunning = false;
    }

    void WorkerThread(int connIndex, const std::wstring host, INTERNET_PORT port, const std::wstring path, DWORD flags, const std::wstring filePath) {
        if (!m_item || connIndex >= (int)m_item->connections.size()) return;

        ConnectionInfo& conn = m_item->connections[connIndex];

        // If chunk is already finished, do nothing!
        uint64_t segLen = (conn.endByte >= conn.startByte) ? (conn.endByte - conn.startByte + 1) : 0;
        if (segLen > 0 && conn.downloadedBytes >= segLen) {
            conn.status = L"Complete";
            return;
        }

        std::wstring actualUA = !m_item->userAgent.empty() ? m_item->userAgent : L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/155.0.0.0 Safari/537.36";
        bool triedUAFallback = false;

        int maxRetries = 10;
        for (int attempt = 1; attempt <= maxRetries && !m_isPaused && !m_isCanceled; attempt++) {
            conn.status = (attempt == 1) ? L"Connecting..." : (L"Reconnecting attempt " + std::to_wstring(attempt) + L"/10...");
            if (m_callback) m_callback->OnProgress(m_item->id);

            HINTERNET hSession = WinHttpOpen(actualUA.c_str(),
                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!hSession) {
                conn.status = L"Error";
                return;
            }

            DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
            WinHttpSetOption(hSession, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

            DWORD secureProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
            WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secureProtocols, sizeof(secureProtocols));
            WinHttpSetTimeouts(hSession, 15000, 20000, 30000, 30000);

            HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
            if (!hConnect) {
                WinHttpCloseHandle(hSession);
                if (attempt < maxRetries && !m_isPaused && !m_isCanceled) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (attempt > 3 ? 3 : attempt)));
                    continue;
                }
                conn.status = L"Connect failed";
                return;
            }

            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
            if (!hRequest) {
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                if (attempt < maxRetries && !m_isPaused && !m_isCanceled) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (attempt > 3 ? 3 : attempt)));
                    continue;
                }
                conn.status = L"Request failed";
                return;
            }

            DWORD http2Flag = WINHTTP_PROTOCOL_FLAG_HTTP2;
            WinHttpSetOption(hRequest, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, &http2Flag, sizeof(http2Flag));

            DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                             SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                             SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                             SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
            WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
            WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

            // Browser headers
            std::wstring browserHdrs = L"User-Agent: " + actualUA + L"\r\n"
                L"Accept: */*\r\n"
                L"Accept-Language: en-US,en;q=0.9\r\n";
            if (!m_item->referer.empty()) {
                browserHdrs += L"Referer: " + m_item->referer + L"\r\n";
            } else if (m_item->url.find(L"googlevideo.com") != std::wstring::npos) {
                browserHdrs += L"Referer: https://www.youtube.com/\r\n";
            }
            if (m_item->url.find(L"googlevideo.com") != std::wstring::npos) {
                browserHdrs += L"Origin: https://www.youtube.com\r\n"
                               L"Sec-Fetch-Dest: video\r\n"
                               L"Sec-Fetch-Mode: cors\r\n"
                               L"Sec-Fetch-Site: cross-site\r\n";
            }
            if (!m_item->cookies.empty()) {
                browserHdrs += L"Cookie: " + m_item->cookies + L"\r\n";
            }
            WinHttpAddRequestHeaders(hRequest, browserHdrs.c_str(), (DWORD)browserHdrs.length(), WINHTTP_ADDREQ_FLAG_ADD);

            // Add Range header for this connection's chunk
            uint64_t currentStart = conn.startByte + conn.downloadedBytes;
            if (conn.endByte >= currentStart) {
                wchar_t rangeHdr[128];
                swprintf_s(rangeHdr, L"Range: bytes=%llu-%llu\r\n", currentStart, conn.endByte);
                WinHttpAddRequestHeaders(hRequest, rangeHdr, (DWORD)wcslen(rangeHdr), WINHTTP_ADDREQ_FLAG_ADD);
            } else if (conn.endByte > 0) {
                // Chunk is already completed
                conn.status = L"Complete";
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return;
            }

            conn.status = L"Send GET...";
            if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
                !WinHttpReceiveResponse(hRequest, NULL)) {
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                if (attempt < maxRetries && !m_isPaused && !m_isCanceled) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (attempt > 3 ? 3 : attempt)));
                    continue;
                }
                conn.status = L"Request failed";
                return;
            }

            DWORD statusCode = 0;
            DWORD scSize = sizeof(statusCode);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &scSize, WINHTTP_NO_HEADER_INDEX);

            if ((statusCode == 403 || statusCode == 406) && !triedUAFallback) {
                triedUAFallback = true;
                actualUA = L"curl/8.4.0";
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                continue;
            }

            if (statusCode != 200 && statusCode != 206) {
                wchar_t stBuf[64];
                swprintf_s(stBuf, L"HTTP %u", statusCode);
                conn.status = stBuf;
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                // If it's a 5xx server error, retry; if 4xx (e.g. 400 link expired or range rejected), don't loop endlessly
                if (statusCode >= 500 && attempt < maxRetries && !m_isPaused && !m_isCanceled) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                    continue;
                }
                return;
            }

            if (statusCode == 200 && conn.startByte > 0) {
                // Server rejected/ignored Range header for multi-stream chunk (returned 200 from byte 0).
                // Do not write byte 0 at this chunk's non-zero offset!
                conn.status = L"Range rejected";
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return;
            }

            HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

            if (hFile == INVALID_HANDLE_VALUE) {
                conn.status = L"File open failed";
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return;
            }

            DWORD dwRet = 0;
            DeviceIoControl(hFile, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dwRet, NULL);

            conn.status = L"Receiving data...";

            std::vector<char> buffer(256 * 1024);
            DWORD bytesRead = 0;
            auto windowStartTime = std::chrono::steady_clock::now();
            uint64_t bytesReadInWindow = 0;
            bool readError = false;

            while (!m_isPaused && !m_isCanceled) {
                // Throttling
                if (m_item && m_item->speedLimitEnabled && m_item->speedLimitKBps > 0) {
                    uint32_t totalLimitKBps = m_item->speedLimitKBps;
                    size_t numWorkers = m_item->connections.empty() ? 1 : m_item->connections.size();
                    uint64_t workerLimitBytes = ((uint64_t)totalLimitKBps * 1024) / numWorkers;
                    if (workerLimitBytes < 1024) workerLimitBytes = 1024;

                    auto now = std::chrono::steady_clock::now();
                    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - windowStartTime).count();

                    if (elapsedMs < 1000 && bytesReadInWindow >= workerLimitBytes) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000 - elapsedMs));
                        windowStartTime = std::chrono::steady_clock::now();
                        bytesReadInWindow = 0;
                    } else if (elapsedMs >= 1000) {
                        windowStartTime = now;
                        bytesReadInWindow = 0;
                    }
                }

                if (!WinHttpReadData(hRequest, buffer.data(), (DWORD)buffer.size(), &bytesRead)) {
                    readError = true;
                    break;
                }
                if (bytesRead == 0) {
                    break; // Complete or server closed
                }

                bytesReadInWindow += bytesRead;

                uint64_t currentPos = conn.startByte + conn.downloadedBytes;
                if (conn.endByte >= conn.startByte) {
                    if (currentPos > conn.endByte) {
                        break;
                    }
                    if (currentPos + bytesRead > conn.endByte + 1) {
                        bytesRead = (DWORD)((conn.endByte + 1) - currentPos);
                    }
                }

                if (bytesRead > 0) {
                    LARGE_INTEGER li;
                    li.QuadPart = currentPos;
                    SetFilePointerEx(hFile, li, NULL, FILE_BEGIN);

                    DWORD bytesWritten = 0;
                    WriteFile(hFile, buffer.data(), bytesRead, &bytesWritten, NULL);

                    conn.downloadedBytes += bytesRead;
                }

                if (conn.endByte >= conn.startByte && (conn.startByte + conn.downloadedBytes) >= (conn.endByte + 1)) {
                    break;
                }
            }

            CloseHandle(hFile);
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);

            uint64_t targetLen = (conn.endByte >= conn.startByte) ? (conn.endByte - conn.startByte + 1) : 0;
            if (targetLen > 0 && conn.downloadedBytes >= targetLen) {
                conn.status = L"Complete";
                return;
            }

            if (m_isPaused) {
                conn.status = L"Paused";
                return;
            }
            if (m_isCanceled) {
                conn.status = L"Canceled";
                return;
            }

            // If connection closed prematurely without error, retry
            if (attempt < maxRetries && !m_isPaused && !m_isCanceled) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }

            conn.status = readError ? L"Connection lost" : L"Incomplete";
            return;
        }
    }
};

DownloadEngine::DownloadEngine() {
    wchar_t appData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
        PathCombineW(appData, appData, L"TafimIDM");
        CreateDirectoryW(appData, NULL);
        wchar_t dataPath[MAX_PATH];
        PathCombineW(dataPath, appData, L"downloads.dat");
        m_dataFile = dataPath;
    }

    // Load max default connections from registry
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Tafim", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwVal = 0, dwSize = sizeof(dwVal);
        if (RegQueryValueExW(hKey, L"MaxConnections", NULL, NULL, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS && dwVal >= 1 && dwVal <= 32) {
            m_defaultNumConnections = (int)dwVal;
        }
        RegCloseKey(hKey);
    }

    LoadDownloads();
}

DownloadEngine::~DownloadEngine() {
    SaveDownloads();
    for (auto& task : m_tasks) {
        if (task) task->Stop();
    }
}

std::shared_ptr<DownloadTask> DownloadEngine::FindTask(int id) {
    for (auto& task : m_tasks) {
        if (task && task->GetId() == id) return task;
    }
    return nullptr;
}

int DownloadEngine::AddDownload(const std::wstring& url, const std::wstring& saveFolder, const std::wstring& customFilename, const std::wstring& category,
    const std::wstring& userAgent, const std::wstring& cookies, const std::wstring& referer,
    const std::wstring& audioUrl, bool isDash, int qualityHeight) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto item = std::make_shared<DownloadItem>();
    item->id = m_nextId++;
    item->url = url;
    item->audioUrl = audioUrl;
    item->isDash = isDash || !audioUrl.empty();
    item->qualityHeight = qualityHeight;
    item->savePath = saveFolder;
    item->category = category;
    item->userAgent = userAgent;
    item->cookies = cookies;
    item->referer = referer;

    if (!customFilename.empty()) {
        item->fileName = customFilename;
    } else {
        // Extract filename from URL
        size_t lastSlash = url.find_last_of(L"/\\");
        if (lastSlash != std::wstring::npos && lastSlash + 1 < url.length()) {
            std::wstring f = url.substr(lastSlash + 1);
            size_t qmark = f.find(L'?');
            if (qmark != std::wstring::npos) f = f.substr(0, qmark);
            item->fileName = f.empty() ? L"download.bin" : f;
        } else {
            item->fileName = L"download.bin";
        }
    }

    // Determine category automatically if default
    std::wstring ext = PathFindExtensionW(item->fileName.c_str());
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    if (ext == L".zip" || ext == L".rar" || ext == L".7z" || ext == L".tar" || ext == L".gz") {
        item->category = L"Compressed";
    } else if (ext == L".exe" || ext == L".msi" || ext == L".bat" || ext == L".cmd") {
        item->category = L"Programs";
    } else if (ext == L".mp4" || ext == L".mkv" || ext == L".avi" || ext == L".webm" || ext == L".ogv" || ext == L".mov") {
        item->category = L"Video";
    } else if (ext == L".mp3" || ext == L".wav" || ext == L".flac" || ext == L".aac" || ext == L".ogg") {
        item->category = L"Music";
    } else if (ext == L".pdf" || ext == L".doc" || ext == L".docx" || ext == L".txt" || ext == L".epub") {
        item->category = L"Documents";
    }

    // Timestamp
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t dateBuf[64];
    swprintf_s(dateBuf, L"%04d-%02d-%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    item->dateAdded = dateBuf;
    item->lastTryDate = dateBuf;

    item->speedLimitEnabled = m_globalSpeedLimitEnabled;
    item->speedLimitKBps = m_globalSpeedLimitKBps;
    item->numConnections = m_defaultNumConnections;

    m_items.push_back(item);
    SaveDownloads();
    return item->id;
}

void DownloadEngine::SetGlobalSpeedLimit(bool enabled, uint32_t kbps) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_globalSpeedLimitEnabled = enabled;
    m_globalSpeedLimitKBps = kbps;
    for (auto& item : m_items) {
        if (item) {
            item->speedLimitEnabled = enabled;
            item->speedLimitKBps = kbps;
        }
    }
}

bool DownloadEngine::StartDownload(int downloadId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto item = GetItem(downloadId);
    if (!item) return false;

    auto task = FindTask(downloadId);
    if (!task) {
        task = std::make_shared<DownloadTask>(item, m_callback);
        m_tasks.push_back(task);
    }
    task->Start();
    return true;
}

bool DownloadEngine::PauseDownload(int downloadId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto task = FindTask(downloadId);
    if (task) {
        task->Pause();
        SaveDownloads();
        return true;
    }
    return false;
}

bool DownloadEngine::ResumeDownload(int downloadId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto task = FindTask(downloadId);
    if (task) {
        task->Start();
        return true;
    } else {
        auto item = GetItem(downloadId);
        if (item) {
            task = std::make_shared<DownloadTask>(item, m_callback);
            m_tasks.push_back(task);
            task->Start();
            return true;
        }
    }
    return false;
}

bool DownloadEngine::CancelDownload(int downloadId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto task = FindTask(downloadId);
    if (task) {
        task->Cancel();
        SaveDownloads();
        return true;
    }
    return false;
}

bool DownloadEngine::DeleteDownload(int downloadId, bool deleteFile) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto task = FindTask(downloadId);
    if (task) {
        task->Stop();
    }
    // Remove from tasks
    m_tasks.erase(std::remove_if(m_tasks.begin(), m_tasks.end(), [downloadId](const std::shared_ptr<DownloadTask>& t) {
        return t && t->GetId() == downloadId;
    }), m_tasks.end());

    // Remove from items
    for (auto it = m_items.begin(); it != m_items.end(); ++it) {
        if ((*it)->id == downloadId) {
            if (deleteFile) {
                wchar_t fullPath[MAX_PATH];
                PathCombineW(fullPath, (*it)->savePath.c_str(), (*it)->fileName.c_str());
                DeleteFileW(fullPath);
            }
            m_items.erase(it);
            SaveDownloads();
            return true;
        }
    }
    return false;
}

bool DownloadEngine::UpdateDownloadUrl(int downloadId, const std::wstring& newUrl) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto item = GetItem(downloadId);
    if (!item || newUrl.empty()) return false;

    auto task = FindTask(downloadId);
    if (task) {
        task->Stop();
    }
    m_tasks.erase(std::remove_if(m_tasks.begin(), m_tasks.end(), [downloadId](const std::shared_ptr<DownloadTask>& t) {
        return t && t->GetId() == downloadId;
    }), m_tasks.end());

    item->url = newUrl;
    if (item->status == DownloadStatus::Error) {
        item->status = DownloadStatus::Paused;
    }
    SaveDownloads();
    return true;
}

std::shared_ptr<DownloadItem> DownloadEngine::GetItem(int downloadId) {
    for (auto& item : m_items) {
        if (item->id == downloadId) return item;
    }
    return nullptr;
}

std::vector<std::shared_ptr<DownloadItem>> DownloadEngine::GetAllItems() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_items;
}

static std::wstring EscapeField(const std::wstring& str) {
    std::wstring out;
    out.reserve(str.size());
    for (wchar_t ch : str) {
        if (ch == L'\t') out += L"\\t";
        else if (ch == L'\r') out += L"\\r";
        else if (ch == L'\n') out += L"\\n";
        else if (ch == L'\\') out += L"\\\\";
        else out += ch;
    }
    return out;
}

static std::wstring UnescapeField(const std::wstring& str) {
    std::wstring out;
    out.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == L'\\' && i + 1 < str.size()) {
            wchar_t next = str[i + 1];
            if (next == L't') { out += L'\t'; ++i; }
            else if (next == L'r') { out += L'\r'; ++i; }
            else if (next == L'n') { out += L'\n'; ++i; }
            else if (next == L'\\') { out += L'\\'; ++i; }
            else { out += str[i]; }
        } else {
            out += str[i];
        }
    }
    return out;
}

void DownloadEngine::SaveDownloads() {
    if (m_dataFile.empty()) return;

    HANDLE hFile = CreateFileW(m_dataFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    // Write UTF-16LE BOM
    WORD bom = 0xFEFF;
    DWORD written = 0;
    WriteFile(hFile, &bom, sizeof(bom), &written, NULL);

    std::wstring content;
    for (const auto& item : m_items) {
        if (!item) continue;
        int statusVal = (int)item->status;
        if (item->status == DownloadStatus::Downloading || item->status == DownloadStatus::Connecting) {
            statusVal = (int)DownloadStatus::Paused;
        }

        std::wstringstream ss;
        ss << item->id << L"\t"
           << EscapeField(item->url) << L"\t"
           << EscapeField(item->fileName) << L"\t"
           << EscapeField(item->savePath) << L"\t"
           << EscapeField(item->category) << L"\t"
           << EscapeField(item->description) << L"\t"
           << EscapeField(item->userAgent) << L"\t"
           << EscapeField(item->cookies) << L"\t"
           << EscapeField(item->referer) << L"\t"
           << item->totalBytes << L"\t"
           << item->downloadedBytes << L"\t"
           << statusVal << L"\t"
           << EscapeField(item->dateAdded) << L"\t"
           << EscapeField(item->lastTryDate) << L"\r\n";
        content += ss.str();
    }

    if (!content.empty()) {
        WriteFile(hFile, content.c_str(), (DWORD)(content.size() * sizeof(wchar_t)), &written, NULL);
    }

    CloseHandle(hFile);
}

void DownloadEngine::LoadDownloads() {
    m_items.clear();
    m_nextId = 1;

    if (m_dataFile.empty() || GetFileAttributesW(m_dataFile.c_str()) == INVALID_FILE_ATTRIBUTES) {
        // Fresh install: starts completely blank!
        return;
    }

    HANDLE hFile = CreateFileW(m_dataFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize < sizeof(WORD)) {
        CloseHandle(hFile);
        return;
    }

    std::vector<BYTE> buf(fileSize + sizeof(wchar_t), 0);
    DWORD readBytes = 0;
    ReadFile(hFile, buf.data(), fileSize, &readBytes, NULL);
    CloseHandle(hFile);

    const wchar_t* pText = (const wchar_t*)buf.data();
    size_t charCount = readBytes / sizeof(wchar_t);
    if (charCount > 0 && pText[0] == 0xFEFF) {
        pText++;
        charCount--;
    }

    std::wstring fullText(pText, charCount);
    std::wstringstream ss(fullText);
    std::wstring line;

    int maxId = 0;
    while (std::getline(ss, line)) {
        while (!line.empty() && (line.back() == L'\r' || line.back() == L'\n')) line.pop_back();
        if (line.empty()) continue;

        std::vector<std::wstring> tokens;
        size_t start = 0;
        while (start < line.size()) {
            size_t tab = line.find(L'\t', start);
            if (tab == std::wstring::npos) {
                tokens.push_back(line.substr(start));
                break;
            } else {
                tokens.push_back(line.substr(start, tab - start));
                start = tab + 1;
            }
        }

        if (tokens.size() >= 14) {
            auto item = std::make_shared<DownloadItem>();
            item->id = _wtoi(tokens[0].c_str());
            item->url = UnescapeField(tokens[1]);
            item->fileName = UnescapeField(tokens[2]);
            item->savePath = UnescapeField(tokens[3]);
            item->category = UnescapeField(tokens[4]);
            item->description = UnescapeField(tokens[5]);
            item->userAgent = UnescapeField(tokens[6]);
            item->cookies = UnescapeField(tokens[7]);
            item->referer = UnescapeField(tokens[8]);
            item->totalBytes = _wcstoui64(tokens[9].c_str(), NULL, 10);
            item->downloadedBytes = _wcstoui64(tokens[10].c_str(), NULL, 10);
            int statusInt = _wtoi(tokens[11].c_str());
            item->status = (DownloadStatus)statusInt;
            item->dateAdded = UnescapeField(tokens[12]);
            item->lastTryDate = UnescapeField(tokens[13]);

            if (item->id > maxId) maxId = item->id;
            m_items.push_back(item);
        }
    }

    m_nextId = maxId + 1;
}
