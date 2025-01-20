#!/bin/bash

# ========= TORCH_PATH =========
cmake_prefix_path_tmp=$1
if [[ -z "$cmake_prefix_path_tmp" ]]; then
    cmake_prefix_path_tmp="subprojects/libtorch/share/cmake"
    echo "No cmake prefix path provided. Using default: $cmake_prefix_path_tmp"
else
    echo "cmake_prefix_path_tmp: $cmake_prefix_path_tmp"
fi

# ========= Architecture =========
# Detect OS and Architecture
OS=$(uname -s)
ARCH=$(uname -m)

echo "$OS - $ARCH"

# ========= clone submodules =========
git submodule update --init --recursive


# ========= libtorch =========
# Determine download URL based on OS and architecture
if [[ "$OS" == "Linux" ]]; then
    LIBTORCH_URL="https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.0%2Bcpu.zip"
elif [[ "$OS" == "Darwin" ]]; then
    if [[ "$ARCH" == "arm64" ]]; then
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
libtorch_zip_file="libtorch.zip"
DEST_DIR="subprojects"
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


# ========= Icon =========
## generate icon for PretextViewAI
## which is already finished and pushed to the repo 
# cd ico_design && cd ico_design &&  iconutil -c icns icon_v2.iconset && cd ..


# ========= CMake compile and install =========
# Finished: there are still problem for installation as the app can not find the LC_RPATH, need to fix this
build_dir="build_cmake"
install_path="app"
if [[ "$OS" == "Darwin" ||  "$OS" == "Linux" ]]; then
    rm -rf ${build_dir} ${install_path} 
else
    echo "Unsupported platform: $OS"
    exit 1
fi

cmake -DCMAKE_BUILD_TYPE=Release -DGLFW_BUILD_WAYLAND=OFF -DGLFW_BUILD_X11=OFF -DWITH_PYTHON=OFF -DCMAKE_INSTALL_PREFIX=${install_path} -DCMAKE_PREFIX_PATH=${cmake_prefix_path_tmp} -S . -B ${build_dir}  && cmake --build ${build_dir} --config Release && cmake --install ${build_dir}

cmake --build build_cmake --target package


# if [[ "$OS" == "Darwin" ]]; then
#     bash ./mac_dmg_generate.sh
# fi


# PretextViewAI.app/Contents/MacOS/PretextViewAI /Users/sg35/auto-curation/log/learning_notes/hic_curation/13 idLinTess1_1\ auto-curation/aPelFus1_1.pretext



# test
# install_name_tool -add_rpath subprojects/libtorch/lib app/bin/PretextView
