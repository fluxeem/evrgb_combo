@echo off
REM EvRGB Combo SDK Documentation Auto Update Script
REM This script will automatically generate documentation, backup old versions, and upload to Alibaba Cloud OSS

setlocal enabledelayedexpansion

REM Set project root directory
set "PROJECT_ROOT=%~dp0../.."

REM Set OSS paths
set "OSS_BASE=oss://fluxeem-hk/evrgb_combo"
set "OSS_EN_DOCS=%OSS_BASE%/en/docs"
set "OSS_ZH_DOCS=%OSS_BASE%/zh/docs"

REM Get version number from version.h file
set "VERSION_FILE=%PROJECT_ROOT%\include\core\version.h"
set "VERSION_MAJOR=0"
set "VERSION_MINOR=0"
set "VERSION_PATCH=0"

REM Parse version numbers
for /f "tokens=3" %%i in ('findstr /C:"#define EVRGB_VERSION_MAJOR" "%VERSION_FILE%"') do set "VERSION_MAJOR=%%i"
for /f "tokens=3" %%i in ('findstr /C:"#define EVRGB_VERSION_MINOR" "%VERSION_FILE%"') do set "VERSION_MINOR=%%i"
for /f "tokens=3" %%i in ('findstr /C:"#define EVRGB_VERSION_PATCH" "%VERSION_FILE%"') do set "VERSION_PATCH=%%i"

set "VERSION=%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%"
echo Current version: %VERSION%

REM Check if doxygen is available
where doxygen >nul 2>&1
if errorlevel 1 (
    echo ERROR: Doxygen is not installed or not in PATH
    pause
    exit /b 1
)

REM Check if ossutil is available
where ossutil64 >nul 2>&1
if errorlevel 1 (
    echo ERROR: ossutil64 is not installed or not in PATH
    pause
    exit /b 1
)

echo Starting documentation update...

REM Step 1: Generate English documentation
echo [1/6] Generating English documentation...
cd /d "%PROJECT_ROOT%"
doxygen documentation\Doxyfile.en
if errorlevel 1 (
    echo ERROR: Failed to generate English documentation
    pause
    exit /b 1
)

REM Step 2: Generate Chinese documentation
echo [2/6] Generating Chinese documentation...
doxygen documentation\Doxyfile.zh
if errorlevel 1 (
    echo ERROR: Failed to generate Chinese documentation
    pause
    exit /b 1
)

REM Step 3: Upload new English documentation
echo [3/6] Uploading new English documentation...
ossutil64 cp -r "%PROJECT_ROOT%\docs\en\." "%OSS_EN_DOCS%" --update --force
if errorlevel 1 (
    echo ERROR: Failed to upload English documentation
    pause
    exit /b 1
)

REM Step 4: Upload new Chinese documentation
echo [4/6] Uploading new Chinese documentation...
ossutil64 cp -r "%PROJECT_ROOT%\docs\zh\." "%OSS_ZH_DOCS%" --update --force
if errorlevel 1 (
    echo ERROR: Failed to upload Chinese documentation
    pause
    exit /b 1
)

REM Step 5: Backup existing English documentation
echo [5/6] Backing up existing English documentation to version %VERSION%...
ossutil64 cp -r "%OSS_EN_DOCS%" "%OSS_BASE%/en/%VERSION%" --update
if errorlevel 1 (
    echo WARNING: Failed to backup English documentation (might not exist)
)

REM Step 6: Backup existing Chinese documentation
echo [6/6] Backing up existing Chinese documentation to version %VERSION%...
ossutil64 cp -r "%OSS_ZH_DOCS%" "%OSS_BASE%/zh/%VERSION%" --update
if errorlevel 1 (
    echo WARNING: Failed to backup Chinese documentation (might not exist)
)

echo.
echo ========================================
echo Documentation update completed!
echo Version: %VERSION%
echo English docs: %OSS_EN_DOCS%
echo Chinese docs: %OSS_ZH_DOCS%
echo Backup location: %OSS_BASE%/en/%VERSION% and %OSS_BASE%/zh/%VERSION%
echo ========================================

pause