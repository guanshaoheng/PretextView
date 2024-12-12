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

# current, no Eigen is needed
# # Eigen: download and compile 
# wget -P subprojects https://gitlab.com/libeigen/eigen/-/archive/3.3.9/eigen-3.3.9.tar.gz
# tar -xzvf subprojects/eigen-3.3.9.tar.gz -C subprojects
# rm subprojects/eigen-3.3.9.tar.gz
# mv subprojects/eigen-3.3.9 subprojects/eigen
# cd subprojects/eigen
# mkdir build && cd build
# cmake .. -DCMAKE_INSTALL_PREFIX=../install_local && cmake --build . --config Release -j 6 && cmake --install .


# generate icon for PretextViewAI
cd ico_design && cd ico_design &&  iconutil -c icns icon_v2.iconset && cd ..


# Finished: there are still problem for installation as the app can not find the LC_RPATH, need to fix this
rm -rf build_cmake  PretextView.app  PretextViewAI.app PretextViewAI.dmg
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=PretextViewAI.app -S . -B build_cmake  && cmake --build build_cmake  -j 8 && cmake --install build_cmake
 
PretextViewAI.app/Contents/MacOS/PretextViewAI /Users/sg35/auto-curation/log/learning_notes/hic_curation/13 idLinTess1_1 auto-curation/aPelFus1_1.pretext
# test
# install_name_tool -add_rpath subprojects/libtorch/lib app/bin/PretextView
