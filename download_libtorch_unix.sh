#!/bin/bash

OS=$(uname -s)
ARCH=$(uname -m)
libtorch_zip_file="libtorch.zip"
DEST_DIR="subprojects"

# ========= libtorch =========
# Determine download URL based on OS and architecture
if [[ "$OS" == "Linux" ]]; then
    LIBTORCH_URL="https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.0%2Bcpu.zip"
elif [[ "$OS" == "Darwin" ]]; then
    if [[ "$ARCH" == "arm64" ]]; then
        LIBTORCH_URL="https://download.pytorch.org/libtorch/cpu/libtorch-macos-arm64-2.5.0.zip"
    elif [[ "$ARCH" == "x86_64" ]]; then
        # LIBTORCH_URL="https://download.pytorch.org/libtorch/cpu/libtorch-macos-x86_64-2.2.2.zip"
        LIBTORCH_URL="https://download.pytorch.org/libtorch/cpu/libtorch-macos-arm64-2.5.0.zip"
    else
        echo "Unsupported architecture: $OS - $ARCH"
        exit 1
    fi
else
    echo "Unsupported platform: $OS"
    exit 1
fi
# Download the selected version
mkdir -p "${DEST_DIR}"

if [[ ! -d "${DEST_DIR}/libtorch" ]]; then
    if [[ ! -f "${libtorch_zip_file}" ]]; then
        echo "libtorch.zip not found. Downloading..."
        curl -o "${libtorch_zip_file}" "$LIBTORCH_URL"
    else
        echo "${libtorch_zip_file} already exists. Skipping download."
    fi
    echo "Extracting libtorch.zip to ${DEST_DIR}/libtorch"
    mkdir "$DEST_DIR"
    unzip -qq "${libtorch_zip_file}" -d "$DEST_DIR"
    echo "Extracted libtorch.zip to ${DEST_DIR}/libtorch"
else
    echo "${DEST_DIR}/libtorch already exists. Skipping extraction."
fi

if [[ -f "${DEST_DIR}/libtorch/share/cmake/Torch/TorchConfig.cmake" ]]; then
    echo "libtorch successfully installed."
else
    echo "libtorch installation failed."
    exit 1
fi

# Clean up
if [ -f "${libtorch_zip_file}" ]; then
    rm ${libtorch_zip_file}
fi