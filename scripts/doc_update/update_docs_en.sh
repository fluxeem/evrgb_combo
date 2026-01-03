#!/bin/bash
# EvRGB Combo SDK Documentation Auto Update Script
# This script will automatically generate documentation, backup old versions, and upload to Alibaba Cloud OSS

set -e  # Exit on error

# Set project root directory
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Set OSS paths
OSS_BASE="oss://fluxeem-hk/evrgb_combo"
OSS_EN_DOCS="${OSS_BASE}/en/docs"
OSS_ZH_DOCS="${OSS_BASE}/zh/docs"

# Get version number from version.h file
VERSION_FILE="${PROJECT_ROOT}/include/core/version.h"
VERSION_MAJOR=$(grep "#define EVRGB_VERSION_MAJOR" "${VERSION_FILE}" | awk '{print $3}')
VERSION_MINOR=$(grep "#define EVRGB_VERSION_MINOR" "${VERSION_FILE}" | awk '{print $3}')
VERSION_PATCH=$(grep "#define EVRGB_VERSION_PATCH" "${VERSION_FILE}" | awk '{print $3}')

VERSION="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
echo "Current version: ${VERSION}"

# Check if doxygen is available
if ! command -v doxygen &> /dev/null; then
    echo "ERROR: Doxygen is not installed or not in PATH"
    exit 1
fi

# Check if ossutil is available
if ! command -v ossutil64 &> /dev/null; then
    echo "ERROR: ossutil64 is not installed or not in PATH"
    exit 1
fi

echo "Starting documentation update..."

# Step 1: Generate English documentation
echo "[1/6] Generating English documentation..."
cd "${PROJECT_ROOT}"
doxygen documentation/Doxyfile.en

# Step 2: Generate Chinese documentation
echo "[2/6] Generating Chinese documentation..."
doxygen documentation/Doxyfile.zh

# Step 3: Upload new English documentation
echo "[3/6] Uploading new English documentation..."
ossutil64 cp -r "${PROJECT_ROOT}/docs/en/." "${OSS_EN_DOCS}" --update --force

# Step 4: Upload new Chinese documentation
echo "[4/6] Uploading new Chinese documentation..."
ossutil64 cp -r "${PROJECT_ROOT}/docs/zh/." "${OSS_ZH_DOCS}" --update --force

# Step 5: Backup existing English documentation
echo "[5/6] Backing up existing English documentation to version ${VERSION}..."
ossutil64 cp -r "${OSS_EN_DOCS}" "${OSS_BASE}/en/${VERSION}" --update || echo "WARNING: Failed to backup English documentation (might not exist)"

# Step 6: Backup existing Chinese documentation
echo "[6/6] Backing up existing Chinese documentation to version ${VERSION}..."
ossutil64 cp -r "${OSS_ZH_DOCS}" "${OSS_BASE}/zh/${VERSION}" --update || echo "WARNING: Failed to backup Chinese documentation (might not exist)"

echo
echo "========================================"
echo "Documentation update completed!"
echo "Version: ${VERSION}"
echo "English docs: ${OSS_EN_DOCS}"
echo "Chinese docs: ${OSS_ZH_DOCS}"
echo "Backup location: ${OSS_BASE}/en/${VERSION} and ${OSS_BASE}/zh/${VERSION}"
echo "========================================"