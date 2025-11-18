@echo off
REM Download and setup PlatformIO Core for bundling with Electron app
REM This script downloads PlatformIO Core and prepares it for bundling

setlocal enabledelayedexpansion

set "RESOURCES_DIR=%~dp0resources"
set "BIN_DIR=%RESOURCES_DIR%\bin"
set "PLATFORMIO_DIR=%RESOURCES_DIR%\platformio"

echo.
echo ==========================================
echo Setting up PlatformIO Core for bundling
echo ==========================================
echo.

REM Create directories
if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"
if not exist "%PLATFORMIO_DIR%" mkdir "%PLATFORMIO_DIR%"

REM Check if Python 3 is available
where python >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [X] Python not found. Please install Python 3.7+ from https://python.org
    pause
    exit /b 1
)

for /f "tokens=*" %%i in ('python --version') do set PYTHON_VERSION=%%i
echo [OK] Found !PYTHON_VERSION!

echo.
echo Installing PlatformIO Core...
python -m pip install --target "%PLATFORMIO_DIR%" --upgrade platformio

if %ERRORLEVEL% NEQ 0 (
    echo [X] Failed to install PlatformIO Core
    pause
    exit /b 1
)

echo [OK] PlatformIO Core installed successfully

echo.
echo Creating wrapper scripts...

REM Windows batch wrapper
(
echo @echo off
echo setlocal
echo.
echo set "SCRIPT_DIR=%%~dp0"
echo set "PLATFORMIO_DIR=%%SCRIPT_DIR%%..\platformio"
echo set "PYTHON=%%PLATFORMIO_DIR%%\Scripts\python.exe"
echo.
echo REM If Python executable doesn't exist, try system Python
echo if not exist "%%PYTHON%%" ^(
echo     set "PYTHON=python"
echo ^)
echo.
echo "%%PYTHON%%" -m platformio %%*
) > "%BIN_DIR%\platformio-win64.bat"

REM Copy batch file as .exe (Windows will execute .bat files)
copy "%BIN_DIR%\platformio-win64.bat" "%BIN_DIR%\platformio-win64.exe" >nul

echo [OK] Wrapper scripts created
echo.
echo ==========================================
echo PlatformIO Core setup complete!
echo ==========================================
echo.
echo Files created:
dir /b "%BIN_DIR%\platformio-*"
echo.
echo PlatformIO Core is ready to be bundled with your Electron app.
echo.
pause

