#!/bin/bash

# Detect OS and Architecture
OS=$(uname -s)
ARCH=$(uname -m)

# Determine download URL based on OS and architecture
if [[ "$OS" == "Linux" ]]; then
    if [[ "$ARCH" == "x86_64" ]]; then
        LIBTORCH_URL="https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.0%2Bcpu.zip"
    else
        echo "Unsupported architecture: $ARCH"
        exit 1
    fi
elif [[ "$OS" == "Darwin" ]]; then
    LIBTORCH_URL="https://download.pytorch.org/libtorch/cpu/libtorch-macos-arm64-2.5.0.zip"
else
    echo "Unsupported OS: $OS"
    exit 1
fi

# Download the selected version
echo "Downloading libtorch from $LIBTORCH_URL"
curl -o libtorch.zip "$LIBTORCH_URL"

# Unzip to a specific directory (e.g., /usr/local/libtorch)
DEST_DIR="subprojects"
mkdir -p "$DEST_DIR"
unzip libtorch.zip -d "$DEST_DIR"

# Clean up
rm libtorch.zip
echo "libtorch has been downloaded and extracted to $DEST_DIR"


# Finished: there are still problem for installation as the app can not find the LC_RPATH, need to fix this
rm -rf build_cmake app  
cmake -S . -B build_cmake -DCMAKE_INSTALL_PREFIX=$(pwd)/app && cmake --build build_cmake --config Release -j 6 && cmake --install build_cmake 
# test
# install_name_tool -add_rpath subprojects/libtorch/lib app/bin/PretextView
app/bin/PretextView