@echo off
set "SRC=%~dp0build\CompassCadence_artefacts\Release\VST3\compass4cadence.vst3"
set "DST=%LOCALAPPDATA%\Programs\Common\VST3\compass4cadence.vst3"
set "LEGACY_DST=%LOCALAPPDATA%\Programs\Common\VST3\Compass Cadence.vst3"

echo Deploying from: %SRC%
echo To: %DST%

if exist "%LEGACY_DST%" (
    rmdir /s /q "%LEGACY_DST%" 2>nul
)

if exist "%DST%" (
    rmdir /s /q "%DST%" 2>nul
)

xcopy /E /I /Y "%SRC%" "%DST%"
if %errorlevel% equ 0 (
    echo [SUCCESS] Deployed compass4cadence VST3 to User VST3 folder!
) else (
    echo [ERROR] Failed to deploy VST3. Error level: %errorlevel%
)

