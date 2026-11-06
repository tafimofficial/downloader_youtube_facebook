@echo off
setlocal

echo ========================================================
echo Building Tafim Internet Download Manager Suite
echo ========================================================

set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community"
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"

if not exist build mkdir build

echo [1/4] Compiling Resources...
cd src
rc.exe /fo "..\build\app.res" app.rc
cd ..
if errorlevel 1 goto error

echo [2/5] Compiling Native Tafim.exe...
cl.exe /O2 /Oi /GL /MD /EHsc /utf-8 /std:c++17 /W3 /DUNICODE /D_UNICODE /I src ^
    /Fo:build\ ^
    src\main.cpp src\MainWindow.cpp src\DownloadPopup.cpp src\DownloadEngine.cpp src\SegmentBar.cpp ^
    build\app.res ^
    /Fe:build\Tafim.exe ^
    /link /SUBSYSTEM:WINDOWS /LTCG /OPT:REF /OPT:ICF ^
    user32.lib gdi32.lib comctl32.lib winhttp.lib gdiplus.lib shell32.lib shlwapi.lib ole32.lib advapi32.lib ws2_32.lib

if errorlevel 1 goto error
copy /Y build\Tafim.exe build\IDMan.exe >nul

echo [3/5] Compiling Native Uninstall.exe...
cl.exe /O2 /Oi /GL /MD /EHsc /utf-8 /std:c++17 /W3 /DUNICODE /D_UNICODE /I src ^
    /Fo:build\ ^
    src\uninstaller.cpp ^
    /Fe:build\Uninstall.exe ^
    /link /SUBSYSTEM:WINDOWS /LTCG /OPT:REF /OPT:ICF ^
    user32.lib shell32.lib shlwapi.lib advapi32.lib

if errorlevel 1 goto error

echo [4/5] Compiling Native TafimHost.exe (Browser Native Messaging Host)...
cl.exe /O2 /Oi /GL /MD /EHsc /utf-8 /std:c++17 /W3 /DUNICODE /D_UNICODE /I src ^
    /Fo:build\ ^
    src\TafimHost.cpp ^
    /Fe:build\TafimHost.exe ^
    /link /SUBSYSTEM:CONSOLE /LTCG /OPT:REF /OPT:ICF ^
    user32.lib shell32.lib shlwapi.lib winhttp.lib

if errorlevel 1 goto error

echo [5/5] Compiling Native Setup_Tafim.exe (Self-Contained Standalone Installer)...
rc.exe /fo "build\installer.res" src\installer.rc
if errorlevel 1 goto error

cl.exe /O2 /Oi /GL /MD /EHsc /utf-8 /std:c++17 /W3 /DUNICODE /D_UNICODE /I src ^
    /Fo:build\ ^
    src\installer.cpp ^
    build\installer.res ^
    /Fe:Setup_Tafim.exe ^
    /link /SUBSYSTEM:WINDOWS /LTCG /OPT:REF /OPT:ICF ^
    user32.lib gdi32.lib comctl32.lib shell32.lib shlwapi.lib ole32.lib advapi32.lib

if errorlevel 1 goto error

set "INSTALL_DIR=%LOCALAPPDATA%\Programs\Tafim"
if exist "%INSTALL_DIR%" (
    echo Stopping running instances for deployment...
    taskkill /F /IM Tafim.exe /IM IDMan.exe /IM TafimHost.exe >nul 2>&1
    ping 127.0.0.1 -n 2 >nul
    echo Deploying updated binaries to %INSTALL_DIR%...
    copy /Y build\Tafim.exe "%INSTALL_DIR%\Tafim.exe" >nul
    copy /Y build\IDMan.exe "%INSTALL_DIR%\IDMan.exe" >nul
    copy /Y build\Uninstall.exe "%INSTALL_DIR%\Uninstall.exe" >nul
    copy /Y build\TafimHost.exe "%INSTALL_DIR%\TafimHost.exe" >nul
    if not exist "%INSTALL_DIR%\extension" mkdir "%INSTALL_DIR%\extension"
    xcopy /E /Y /I extension "%INSTALL_DIR%\extension" >nul
    echo Deployment completed!
    start "" "%INSTALL_DIR%\Tafim.exe" /background
)

:: Clean up any stray intermediate binaries in root so only Setup_Tafim.exe exists
if exist "Tafim.exe" del /F /Q "Tafim.exe" >nul 2>&1
if exist "IDMan.exe" del /F /Q "IDMan.exe" >nul 2>&1
if exist "Uninstall.exe" del /F /Q "Uninstall.exe" >nul 2>&1
if exist "TafimHost.exe" del /F /Q "TafimHost.exe" >nul 2>&1

echo ========================================================
echo BUILD SUCCESSFUL!
echo Generated Single Executable:
echo   - Setup_Tafim.exe (Only single native installer)
echo ========================================================
exit /b 0

:error
echo [!] BUILD FAILED. Check errors above.
exit /b 1
