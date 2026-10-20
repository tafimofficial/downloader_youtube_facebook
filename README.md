Tafim Downloader
A lightweight downloader built with C. This repository includes the
browser extension used to connect a supported browser to the Tafim
Downloader desktop application.
Note: Update the download links, supported platforms, and
connection details below to match the actual release. The README does
not imply that an installer or extension package exists until it is
published in Releases.

Download
Get the latest build from the project's GitHub Releases page:
- Windows app: download the installer or executable attached to
  the latest release.
- Browser extension: use the extension/ folder in this
  repository, or download an extension ZIP if one is attached to the
  release.
Recommended release assets - Tafim-Downloader-Setup.exe ---
Windows installer, if your build produces one. - Tafim-Downloader.exe
--- portable executable, if provided. - extension.zip --- packaged
browser extension, if provided.
Install the Windows application
1. Open the repository's Releases page.
2. Download the Windows installer or executable attached to the latest
   release.
3. Run the downloaded file and follow the installer instructions, if
   applicable.
4. Launch Tafim Downloader.
5. If Windows Firewall asks for permission, allow access only on
   trusted Private networks, if required by the app.
If a release provides only a portable .exe, there may be no
installation wizard: save the file in a suitable folder and run it.
Install the browser extension
The extension source is stored in the extension/ directory.
Chrome
1. Download and extract the repository ZIP, or clone the repository.
2. Open chrome://extensions.
3. Enable Developer mode.
4. Click Load unpacked.
5. Select the extension/ folder that contains manifest.json.
Microsoft Edge
1. Open edge://extensions.
2. Enable Developer mode.
3. Click Load unpacked.
4. Select the extension/ folder that contains manifest.json.
If an extension.zip is provided in Releases, extract it first and
select the folder containing manifest.json.
Connect the extension to Tafim Downloader
1. Start the Tafim Downloader desktop application.
2. Confirm the extension is installed and enabled.
3. Open the extension from the browser toolbar.
4. Follow the extension's connection instructions to connect it to the
   running desktop app.
The exact connection method depends on the implementation. If the app
uses a local HTTP endpoint, native messaging, or a local WebSocket, the
desktop app and extension must be configured for the same method and
compatible versions.
