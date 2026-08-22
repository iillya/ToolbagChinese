@echo off
setlocal
if not exist "%~dp0..\..\build" mkdir "%~dp0..\..\build"
if not exist "%~dp0..\..\dist" mkdir "%~dp0..\..\dist"
set "VCVARS="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find VC\Auxiliary\Build\vcvars64.bat`) do if not defined VCVARS set "VCVARS=%%i"
)
if not defined VCVARS if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS (
    echo [ERROR] Visual Studio C++ build tools were not found.
    exit /b 1
)
call "%VCVARS%" >nul
rc /nologo /fo "%~dp0..\..\build\app_icon.res" "%~dp0..\resources\app_icon.rc"
if errorlevel 1 exit /b 1
cl /nologo /O2 /W4 /LD /EHa /std:c++20 /utf-8 "%~dp0..\src\dllmain.cpp" user32.lib shell32.lib gdi32.lib /Fo:"%~dp0..\..\build\dllmain.obj" /Fe:"%~dp0..\..\build\ToolbagChineseHook.dll" /link /IMPLIB:"%~dp0..\..\build\ToolbagChineseHook.lib"
if errorlevel 1 exit /b 1
cl /nologo /O2 /W4 /EHsc /utf-8 "%~dp0..\src\launcher.cpp" user32.lib "%~dp0..\..\build\app_icon.res" /Fo:"%~dp0..\..\build\launcher.obj" /link /SUBSYSTEM:WINDOWS /OUT:"%~dp0..\..\build\ToolbagChineseLauncher.exe"
if errorlevel 1 exit /b 1
cl /nologo /O2 /W4 /EHsc /utf-8 "%~dp0..\src\installer.cpp" user32.lib gdi32.lib ole32.lib shell32.lib "%~dp0..\..\build\app_icon.res" /Fo:"%~dp0..\..\build\installer.obj" /link /SUBSYSTEM:WINDOWS /OUT:"%~dp0..\..\build\ChineseInstaller.exe" /MANIFEST:EMBED /MANIFESTUAC:"level='requireAdministrator' uiAccess='false'"
if errorlevel 1 exit /b 1
echo Build complete.
endlocal
