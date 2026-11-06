#pragma once
#include "Common.h"
#include "DownloadItem.h"

#define WC_SEGMENTBAR L"IDMSegmentBar"

void RegisterSegmentBarClass(HINSTANCE hInstance);
HWND CreateSegmentBar(HWND hParent, HINSTANCE hInstance, int id, int x, int y, int w, int h);
void SegmentBar_SetData(HWND hWnd, const std::vector<ConnectionInfo>& conns, uint64_t totalBytes);
