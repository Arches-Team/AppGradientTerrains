@echo off
:: Copies glew32.dll and the Qt libraries next to the executables built by the Visual Studio solution
:: (Out\x64\Debug and Out\x64\Release). QTDIR64 must point to the Qt 6 folder (for example D:\Qt6\6.10.1\msvc2022_64).
:: The script can be started from any folder: all the paths are relative to its own location.

setlocal
set "ROOT=%~dp0.."
set "GLEW=%ROOT%\Libs\LibPlatform\LibGL\x64\bin\glew32.dll"

if not defined QTDIR64 (
    echo QTDIR64 is not defined: set it to the Qt 6 folder, e.g. D:\Qt6\6.10.1\msvc2022_64
    exit /b 1
)
if not exist "%GLEW%" (
    echo glew32.dll not found: %GLEW%
    exit /b 1
)

:: GL
for %%C in (Debug Release) do (
    if not exist "%ROOT%\Out\x64\%%C" mkdir "%ROOT%\Out\x64\%%C"
    copy /Y "%GLEW%" "%ROOT%\Out\x64\%%C\" >nul
)

:: Deploy
if exist "%ROOT%\Out\x64\Release\AppGradient.exe" "%QTDIR64%\bin\windeployqt.exe" --release "%ROOT%\Out\x64\Release\AppGradient.exe"
if exist "%ROOT%\Out\x64\Debug\AppGradient.exe" "%QTDIR64%\bin\windeployqt.exe" "%ROOT%\Out\x64\Debug\AppGradient.exe"

for %%C in (Debug Release) do (
    if exist "%ROOT%\Out\x64\%%C\imageformats" xcopy /Y /I "%ROOT%\Out\x64\%%C\imageformats" "%ROOT%\Out\x64\%%C\plugins\imageformats" >nul
)

endlocal
