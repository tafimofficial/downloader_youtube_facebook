#include "SegmentBar.h"

struct SegmentBarData {
    std::vector<ConnectionInfo> connections;
    uint64_t totalBytes = 0;
};

static LRESULT CALLBACK SegmentBarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SegmentBarData* pData = (SegmentBarData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        pData = new SegmentBarData();
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pData);
        return 0;
    }
    case WM_DESTROY: {
        if (pData) {
            delete pData;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
        }
        return 0;
    }
    case WM_ERASEBKGND:
        return 1; // Double buffered in WM_PAINT

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        // Double buffer DC
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

        // 1. Draw background (light IDM gray/white)
        HBRUSH bgBrush = CreateSolidBrush(RGB(245, 247, 250));
        FillRect(memDC, &rc, bgBrush);
        DeleteObject(bgBrush);

        // 2. Draw border (sunken 3D effect like IDM)
        RECT borderRc = rc;
        DrawEdge(memDC, &borderRc, EDGE_SUNKEN, BF_RECT);

        // Inset by 1-2 pixels for the content area
        InflateRect(&borderRc, -2, -2);
        int usableW = borderRc.right - borderRc.left;
        int barH = borderRc.bottom - borderRc.top;

        if (pData && !pData->connections.empty() && pData->totalBytes > 0) {
            uint64_t total = pData->totalBytes;

            for (size_t i = 0; i < pData->connections.size(); i++) {
                const auto& conn = pData->connections[i];
                if (conn.endByte < conn.startByte) continue;

                uint64_t segStart = conn.startByte;
                uint64_t segEnd = conn.endByte;
                uint64_t segLen = (segEnd >= segStart) ? (segEnd - segStart + 1) : 1;

                int segLeft = borderRc.left + (int)(((double)segStart / (double)total) * usableW);
                int segRight = borderRc.left + (int)(((double)(segEnd + 1) / (double)total) * usableW);
                if (segRight > borderRc.right) segRight = borderRc.right;

                int segDownloadedW = 0;
                if (segLen > 0) {
                    double pct = (double)conn.downloadedBytes / (double)segLen;
                    if (pct > 1.0) pct = 1.0;
                    segDownloadedW = (int)(pct * (segRight - segLeft));
                }

                // Fill connection slot background (subtle light grid)
                RECT slotRc = { segLeft, borderRc.top, segRight, borderRc.bottom };

                // Draw downloaded chunk in vibrant glossy IDM blue
                if (segDownloadedW > 0) {
                    RECT chunkRc = { segLeft, borderRc.top, segLeft + segDownloadedW, borderRc.bottom };

                    // Vertical gradient: Cyan-blue (RGB(30, 144, 255)) to Royal blue (RGB(0, 60, 180))
                    int chunkH = chunkRc.bottom - chunkRc.top;
                    for (int y = 0; y < chunkH; y++) {
                        double factor = (double)y / (double)(chunkH > 1 ? chunkH - 1 : 1);
                        int r = (int)(40 * (1.0 - factor) + 10 * factor);
                        int g = (int)(150 * (1.0 - factor) + 60 * factor);
                        int b = (int)(255 * (1.0 - factor) + 200 * factor);

                        // Highlight line near top for 3D metallic feel
                        if (y == 1 || y == 2) {
                            r = min(255, r + 70);
                            g = min(255, g + 60);
                            b = 255;
                        }

                        HPEN hLinePen = CreatePen(PS_SOLID, 1, RGB(r, g, b));
                        HPEN hOldPen = (HPEN)SelectObject(memDC, hLinePen);
                        MoveToEx(memDC, chunkRc.left, chunkRc.top + y, NULL);
                        LineTo(memDC, chunkRc.right, chunkRc.top + y);
                        SelectObject(memDC, hOldPen);
                        DeleteObject(hLinePen);
                    }
                }

                // Draw Red head indicator line (classic IDM red tick at the current download boundary)
                int headX = segLeft + segDownloadedW;
                if (headX >= segLeft && headX <= segRight) {
                    HPEN redPen = CreatePen(PS_SOLID, 1, RGB(220, 20, 20));
                    HPEN hOldPen = (HPEN)SelectObject(memDC, redPen);
                    MoveToEx(memDC, headX, borderRc.top, NULL);
                    LineTo(memDC, headX, borderRc.bottom);
                    SelectObject(memDC, hOldPen);
                    DeleteObject(redPen);
                }

                // Draw subtle dark divider at end of connection segment
                if (i < pData->connections.size() - 1) {
                    HPEN divPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
                    HPEN hOldPen = (HPEN)SelectObject(memDC, divPen);
                    MoveToEx(memDC, segRight, borderRc.top, NULL);
                    LineTo(memDC, segRight, borderRc.bottom);
                    SelectObject(memDC, hOldPen);
                    DeleteObject(divPen);
                }
            }
        } else if (pData && pData->totalBytes == 0) {
            // Draw empty or indeterminate animation
            HBRUSH emptyBrush = CreateSolidBrush(RGB(230, 230, 230));
            FillRect(memDC, &borderRc, emptyBrush);
            DeleteObject(emptyBrush);
        }

        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
        return 0;
    }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void RegisterSegmentBarClass(HINSTANCE hInstance) {
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = SegmentBarProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = WC_SEGMENTBAR;
    RegisterClassEx(&wc);
}

HWND CreateSegmentBar(HWND hParent, HINSTANCE hInstance, int id, int x, int y, int w, int h) {
    return CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_SEGMENTBAR,
        L"",
        WS_CHILD | WS_VISIBLE,
        x, y, w, h,
        hParent,
        (HMENU)(INT_PTR)id,
        hInstance,
        NULL
    );
}

void SegmentBar_SetData(HWND hWnd, const std::vector<ConnectionInfo>& conns, uint64_t totalBytes) {
    SegmentBarData* pData = (SegmentBarData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (pData) {
        pData->connections = conns;
        pData->totalBytes = totalBytes;
        InvalidateRect(hWnd, NULL, FALSE);
    }
}
