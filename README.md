# Tafim Internet Download Manager 2.0

A high-performance, native Win32 C/C++ Internet Download Manager featuring dynamic segment splitting, multi-connection acceleration, video sniffing, and seamless browser integration.

---

## Key Features

- **Blazing Fast Multi-Connection Acceleration**:
  - Configurable connections (1, 2, 4, 8, 16, 24, 32 parallel threads).
  - **Dynamic Segment Splitting**: Automatically detects when one thread completes and splits the largest remaining chunk so all connections stay green and download at maximum line speed until 100%.
- **Zero Disk Bloat & Instant File Allocation**:
  - Uses NTFS Sparse Files (`FSCTL_SET_SPARSE`) so files allocate disk sectors only as data arrives—no SSD lockups or artificial storage bloat even when downloading 30GB–100GB files.
- **Advanced Cloudflare & WAF Bypass**:
  - Intelligent TLS & User-Agent fallback engine (`curl/8.4.0` / browser headers) to bypass Cloudflare 403 blocks and JA3 fingerprint mismatches.
- **Web Video Sniffing & DASH Demuxing**:
  - Complete support for YouTube, Facebook, Instagram, TikTok, Twitter/X, Vimeo, Dailymotion, and Google Drive.
  - Automatic DASH audio and video stream merging using native pipes.
- **Clipboard URL Monitoring**:
  - Automatically detects downloadable links copied to the clipboard and prompts for instant one-click download.
- **Built-in File Checksum & Integrity Calculator**:
  - Instant MD5 and SHA-256 calculation with one-click copy and live hash verification against expected hashes.
- **Post-Download Automation**:
  - Turn off / shutdown computer when all active downloads finish (with 15-second safety countdown).
  - Optional sound effect on download complete.
- **Bandwidth Control & Speed Limiter**:
  - Real-time KB/s throttling per task or globally.
- **Browser Extension Integration**:
  - High-speed local loopback server + Native Messaging host (`TafimHost.exe`) for Chrome, Edge, Brave, and other Chromium browsers.

---

## Installation

### Option 1: Standalone Installer (Recommended)
1. Run `Setup_Tafim.exe`.
2. Follow the wizard to install Tafim IDM to `%LOCALAPPDATA%\Programs\Tafim`.
3. The installer registers the desktop app, tray icon, file associations, and native browser host automatically.

### Option 2: Browser Extension Setup
1. In Chrome / Edge / Brave, navigate to `chrome://extensions`.
2. Turn on **Developer mode**.
3. Click **Load unpacked** and select the `extension/` folder in this repository.
4. Run `register_extension.bat` as Administrator to link the extension to Tafim IDM.

---

## Building from Source

### Prerequisites
- Windows 10 / 11 (x64)
- Visual Studio 2022 (with "Desktop development with C++" workload)

### Build Command
Run `build.bat` in a Developer Command Prompt or normal terminal:
```cmd
build.bat
```
The script automatically invokes MSVC `cl.exe`, compiles all native executables (`Tafim.exe`, `Uninstall.exe`, `TafimHost.exe`), and packages the self-contained `Setup_Tafim.exe`.
