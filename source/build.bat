@echo off
setlocal
set "ROOT=%~dp0.."
set "SOURCE=%ROOT%\source"
set "BUILD=%ROOT%\build"
set "OUT=%BUILD%\out"
set "OBJ=%BUILD%\obj"
set "DIST=%ROOT%\dist"
set "ICON=%ROOT%\icon"
set "THIRD_PARTY=%ROOT%\third_party"
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%OBJ%" mkdir "%OBJ%"
if not exist "%DIST%" mkdir "%DIST%"
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
rc /nologo /fo "%OBJ%\app_icon.res" "%ICON%\app_icon.rc"
if errorlevel 1 exit /b 1
cl /nologo /O2 /Gy /Gw /W4 /wd4201 /TC /DZYDIS_STATIC_BUILD /I"%THIRD_PARTY%\zydis" /c "%THIRD_PARTY%\zydis\Zydis.c" /Fo:"%OBJ%\zydis.obj"
if errorlevel 1 exit /b 1
cl /nologo /O2 /W4 /LD /EHa /std:c++20 /utf-8 /DZYDIS_STATIC_BUILD /I"%THIRD_PARTY%\zydis" "%SOURCE%\hook.cpp" "%OBJ%\zydis.obj" user32.lib shell32.lib gdi32.lib /Fo:"%OBJ%\hook.obj" /Fe:"%OUT%\ToolbagChineseHook.dll" /link /IMPLIB:"%OBJ%\ToolbagChineseHook.lib"
if errorlevel 1 exit /b 1
cl /nologo /O2 /W4 /EHsc /utf-8 "%SOURCE%\launcher.cpp" user32.lib "%OBJ%\app_icon.res" /Fo:"%OBJ%\launcher.obj" /link /SUBSYSTEM:WINDOWS /OUT:"%OUT%\ToolbagChineseLauncher.exe"
if errorlevel 1 exit /b 1
cl /nologo /O2 /W4 /EHsc /utf-8 "%SOURCE%\installer.cpp" user32.lib gdi32.lib ole32.lib shell32.lib "%OBJ%\app_icon.res" /Fo:"%OBJ%\installer.obj" /link /SUBSYSTEM:WINDOWS /OUT:"%OUT%\ChineseInstaller.exe" /MANIFEST:EMBED /MANIFESTUAC:"level='requireAdministrator' uiAccess='false'"
if errorlevel 1 exit /b 1
python "%SOURCE%\embed_files.py"
if errorlevel 1 exit /b 1
echo Build complete.
endlocal
