

# CMAKE
export PATH=/opt/homebrew/bin:$PATH

builddir="./build_cmake"

if [ -d ${builddir} ]; then
    rm -rf ${builddir}
fi

# compiling the glfw library 
cmake -S ./ -B ${builddir}
cd ${builddir} && make -j 13
