@echo off
echo ========================================================
echo Deploying Compass Cadence VST3 to Program Files (Admin)
echo ========================================================

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] Please right-click this script and select "Run as administrator".
    pause
    exit /b 1
)

set "SRC=%~dp0build\CompassCadence_artefacts\Release\VST3\Compass Cadence.vst3"
set "DST=C:\Program Files\Common Files\VST3\Compass Cadence.vst3"

if exist "%DST%" (
    echo Removing previous build from %DST%...
    rmdir /s /q "%DST%" 2>nul
    del /f /q "%DST%" 2>nul
)

echo Copying latest VST3 bundle to %DST%...
xcopy /e /i /y "%SRC%" "%DST%"

if %errorlevel% equ 0 (
    echo.
    echo [SUCCESS] Compass Cadence VST3 successfully deployed to Program Files!
) else (
    echo.
    echo [ERROR] Failed to copy files.
)

pause
