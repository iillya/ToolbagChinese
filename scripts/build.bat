@echo off
if not exist "%~dp0..\build" mkdir "%~dp0..\build"
if not exist "%~dp0..\dist" mkdir "%~dp0..\dist"
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /O2 /LD /EHsc /EHa /utf-8 "%~dp0..\src\dllmain.cpp" /Fo:"%~dp0..\build\dllmain.obj" /Fe:"%~dp0..\dist\ToolbagChineseHook.dll" /link /IMPLIB:"%~dp0..\build\ToolbagChineseHook.lib"
cl /nologo /O2 /EHsc /utf-8 "%~dp0..\src\launcher.cpp" user32.lib /Fo:"%~dp0..\build\launcher.obj" /link /SUBSYSTEM:WINDOWS /OUT:"%~dp0..\dist\ToolbagChineseLauncher.exe"
