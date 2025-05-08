#!/bin/bash


# ========= Architecture =========
# Detect OS and Architecture
OS=$(uname -s)
ARCH=$(uname -m)

echo "$OS - $ARCH"
# ========= paramter cofig =========
FORCE_MAC_X86=false
BUILD_UNIVERSAL=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --force-x86)
            FORCE_MAC_X86=true
            ARCH="x86_64"
            shift
            ;;
        --universal)
            BUILD_UNIVERSAL=true
            shift
            ;;
        *)
            echo "Not known parameter: $1"
            exit 1
            ;;
    esac
done
echo "After mofidifying: $OS - $ARCH"

# ========= libtorch path =========
cmake_prefix_path_tmp="subprojects/libtorch/share/cmake"
echo "Default Torch path: $cmake_prefix_path_tmp"


# ========= clone submodules =========
git submodule update --init --recursive

# ========= libtorch =========
# current version not needed
# ./download_libtorch_unix.sh


# ========= Icon =========
## generate icon for PretextViewAI
## which is already finished and pushed to the repo 
# cd ico_design && cd ico_design &&  iconutil -c icns icon_v2.iconset && cd ..


# ========= fmt =========
cd subprojects/fmt
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=${ARCH} -S . -B build

cmake --build build --target fmt --config Release || {
    echo "fmt: compile failed!"
    exit 1
}
cd ../../


# ========= libdeflate =========
cd subprojects/libdeflate
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=${ARCH} -S . -B build

cmake --build build --target libdeflate_static --config Release || {
    echo "libdeflate: compile failed!"
    exit 1
}
cd ../../


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

CMAKE_OPTIONS=(
    -DCMAKE_BUILD_TYPE=Release
    -DGLFW_BUILD_WAYLAND=OFF
    # -DGLFW_BUILD_X11=OFF
    -DWITH_TORCH=OFF
    -DCMAKE_INSTALL_PREFIX="$install_path"
    -DCMAKE_PREFIX_PATH="$cmake_prefix_path_tmp"
    -DCMAKE_OSX_ARCHITECTURES=${ARCH}
    -DPYTHON_SCOPED_INTERPRETER=OFF
)
if [[ "$OS" == "Darwin" ]]; then
    if [[ "$FORCE_MAC_X86" == true ]]; then
        CMAKE_OPTIONS+=(
            -DFORCE_MAC_X86=ON
        )
    elif [[ "$BUILD_UNIVERSAL" == true ]]; then
        CMAKE_OPTIONS+=(
            -DBUILD_UNIVERSAL=ON
        )
    fi
fi

cmake "${CMAKE_OPTIONS[@]}" -S . -B ${build_dir}  || {
    echo "PretextView: configure failed!"
    exit 1
}

cmake --build ${build_dir} --target package || {
    echo "PretextView: compile failed!"
    exit 1
}


# PretextViewAI.app/Contents/MacOS/PretextViewAI /Users/sg35/auto-curation/log/learning_notes/hic_curation/13 idLinTess1_1\ auto-curation/aPelFus1_1.pretext



# test
# install_name_tool -add_rpath subprojects/libtorch/lib app/bin/PretextView
