@echo off
setlocal
set "ROOT=%~dp0..\.."
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist "%ROOT%\build\tests" mkdir "%ROOT%\build\tests"
cl /nologo /O2 /W4 /WX /EHa /std:c++20 /utf-8 /DZYDIS_STATIC_BUILD /I"%ROOT%\third_party\zydis" "%~dp0runtime.cpp" "%ROOT%\build\obj\zydis.obj" user32.lib shell32.lib gdi32.lib /Fo:"%ROOT%\build\tests\runtime.obj" /Fe:"%ROOT%\build\tests\runtime.exe"
if errorlevel 1 exit /b 1
python "%~dp0dictionary_corpus.py"
if errorlevel 1 exit /b 1
"%ROOT%\build\tests\runtime.exe" "%ROOT%\translations\dictionary_zh.json" "%ROOT%\build\tests\dictionary-corpus.bin"
if errorlevel 1 exit /b 1
cl /nologo /O2 /W4 /WX /EHsc /std:c++17 /utf-8 "%~dp0association.cpp" advapi32.lib shell32.lib shlwapi.lib /Fo:"%ROOT%\build\tests\association.obj" /Fe:"%ROOT%\build\tests\association.exe"
if errorlevel 1 exit /b 1
"%ROOT%\build\tests\association.exe"
exit /b %errorlevel%
