@echo off
rem ============================================================
rem  Toolbag 汉化插件构建脚本
rem  自动探测 Visual Studio（vswhere），找不到再回退到常见路径
rem ============================================================
setlocal enabledelayedexpansion

if not exist "%~dp0..\..\build" mkdir "%~dp0..\..\build"
if not exist "%~dp0..\..\dist"  mkdir "%~dp0..\..\dist"

rem ---- 定位 vcvars64.bat ----
set "VCVARS="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find VC\Auxiliary\Build\vcvars64.bat`) do (
        if not defined VCVARS set "VCVARS=%%i"
    )
)
if not defined VCVARS if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS (
    echo [ERROR] 找不到 vcvars64.bat。请先安装 "Visual Studio 2022 生成工具（含 C++ 生成工具）"。
    exit /b 1
)

call "%VCVARS%" >nul

rem ---- Hook DLL ----
cl /nologo /O2 /LD /EHa /utf-8 "%~dp0..\src\dllmain.cpp" /Fo:"%~dp0..\..\build\dllmain.obj" /Fe:"%~dp0..\..\build\ToolbagChineseHook.dll" /link /IMPLIB:"%~dp0..\..\build\ToolbagChineseHook.lib"
if errorlevel 1 exit /b 1

rem ---- 汉化启动器（启动 Toolbag 用）----
cl /nologo /O2 /EHsc /utf-8 "%~dp0..\src\launcher.cpp" user32.lib /Fo:"%~dp0..\..\build\launcher.obj" /link /SUBSYSTEM:WINDOWS /OUT:"%~dp0..\..\build\ToolbagChineseLauncher.exe"
if errorlevel 1 exit /b 1

rem ---- 一键安装器（EXE，带提权清单）----
cl /nologo /O2 /EHsc /utf-8 "%~dp0..\src\installer.cpp" user32.lib /Fo:"%~dp0..\..\build\installer.obj" /link /SUBSYSTEM:CONSOLE /OUT:"%~dp0..\..\build\ChineseInstaller.exe" /MANIFEST:EMBED /MANIFESTUAC:"level='requireAdministrator' uiAccess='false'"
if errorlevel 1 exit /b 1

echo 构建完成：build\ToolbagChineseHook.dll 与 build\ToolbagChineseLauncher.exe 与 build\ChineseInstaller.exe
endlocal
