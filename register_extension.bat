@echo off
setlocal

echo ========================================================
echo Registering Tafim Browser Extension Host for Chrome and Edge
echo ========================================================

set "CUR_DIR=%~dp0"
set "HOST_EXE=%CUR_DIR%TafimHost.exe"
set "JSON_FILE=%CUR_DIR%extension\com.tafim.downloader.json"

:: Escape backslashes for JSON
set "ESCAPED_EXE=%HOST_EXE:\=\\%"

echo Updating extension\com.tafim.downloader.json...
(
echo {
echo   "name": "com.tafim.downloader",
echo   "description": "Tafim Internet Download Manager Host",
echo   "path": "%ESCAPED_EXE%",
echo   "type": "stdio",
echo   "allowed_origins": [ "chrome-extension://*/*" ]
echo }
) > "%JSON_FILE%"

echo Registering in Chrome Registry...
reg add "HKCU\Software\Google\Chrome\NativeMessagingHosts\com.tafim.downloader" /ve /t REG_SZ /d "%JSON_FILE%" /f >nul

echo Registering in Edge Registry...
reg add "HKCU\Software\Microsoft\Edge\NativeMessagingHosts\com.tafim.downloader" /ve /t REG_SZ /d "%JSON_FILE%" /f >nul

echo ========================================================
echo Extension Host registered successfully!
echo Now Chrome and Edge will automatically forward downloads to Tafim!
echo ========================================================
pause
