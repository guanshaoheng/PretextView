
# set var
cwd="/Users/sg35/PretextView"

if [ -d "${cwd}/subprojects/glfw/build" ]; then
    rm -rf ${cwd}/subprojects/glfw/build
    echo 
    echo "============================"
    echo "${cwd}/subprojects/glfw/build is deleted"
    echo "============================"
    echo
fi 

# CMAKE
export PATH=/opt/homebrew/bin:$PATH

# compiling the glfw library 
cmake -S ${cwd}/subprojects/glfw -B ${cwd}/subprojects/glfw/build -DBUILD_SHARED_LIBS=ON  # -DBUILD_SHARED_LIBS=ON -DGLFW_LIBRARY_TYPE=SHARED # configure the cmake
cd ${cwd}/subprojects/glfw/build && make -j 13 && cd ${cwd}  # make the glfw library 


# compile the libdeflate dependencies
cd /Users/sg35/PretextView/libdeflate
make clean 
make -j 13


