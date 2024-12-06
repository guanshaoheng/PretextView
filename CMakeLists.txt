cmake_minimum_required(VERSION 3.17)
project(PretextView VERSION "0.2.5")


if (APPLE)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        # Actions for arm64 architecture
        set(CMAKE_OSX_ARCHITECTURES "arm64")
        message(STATUS "Arch: ${CMAKE_SYSTEM_PROCESSOR} thus build for ARM64 architecture.")
        # Set other ARM64-specific variables or flags here
    else()
        # Actions for other architectures (e.g., x86_64)
        set(CMAKE_OSX_ARCHITECTURES "x86_64")
        message(STATUS "Arch: ${CMAKE_SYSTEM_PROCESSOR} thus build for x86_64 architecture.")
        # Set other x86_64-specific variables or flags here
    endif()
endif()


set(target_name PretextView)

set(CMAKE_C_STANDARD 17)   # 设置cpp版本
set(CMAKE_CXX_STANDARD 17)   # 设置cpp版本
set(CMAKE_C_STANDARD_REQUIRED ON)  # 设置cpp版本
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # 设置cpp版本

set(CMAKE_BUILD_TYPE Debug)  # 设置调试版本

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug version")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
else()
    message(STATUS "Release version")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()


set(source_file PretextView.cpp glad.c)
include_directories(
    include
    src
    subprojects/imgui
    subprojects/glfw/include
    subprojects/libdeflate
    subprojects/glm
    subprojects/OpenBLAS
    subprojects/OpenBLAS/build
)


add_subdirectory(subprojects/glfw)
add_subdirectory(subprojects/libdeflate)
add_subdirectory(subprojects/glm)

find_library(OPENBLAS_LIB openblas PATHS subprojects/OpenBLAS/build/lib)
if(NOT OPENBLAS_LIB)
    message(FATAL_ERROR "OpenBLAS not found")
else()
    message(STATUS "OpenBLAS found: ${OPENBLAS_LIB}")
endif()


# set(CMAKE_PREFIX_PATH "subprojects/eigen/install_local/share/eigen3/cmake" ${CMAKE_PREFIX_PATH})
# find_package(Eigen3 REQUIRED NO_MODULE)
# if (NOT Eigen3_FOUND)
#     message(FATAL_ERROR "Eigen3 not found")
# else()
#     message(STATUS "Eigen3 found, and include dir: ${EIGEN3_INCLUDE_DIR}")
# endif()
# include_directories(${EIGEN3_INCLUDE_DIR})

set(CMAKE_PREFIX_PATH "subprojects/libtorch/share/cmake/Torch")
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")


find_library(PYTORCH_EXTENSION_LIBRARY
    NAMES libtorchscatter.dylib
    PATHS subprojects/pytorch_scatter/build
    REQUIRED)
message(STATUS "Pytorch scatter found: ${PYTORCH_EXTENSION_LIBRARY}")


add_library(aisort STATIC src/aisort.cpp )
target_link_libraries(aisort PRIVATE ${TORCH_LIBRARIES} ${PYTORCH_EXTENSION_LIBRARY})


add_library(glad STATIC glad.c)
add_library(utilsPretextView STATIC src/utilsPretextView.cpp)
add_library(copy_texture STATIC src/copy_texture.cpp)
# target_link_libraries(copy_texture PRIVATE Eigen3::Eigen)
add_library(imgui STATIC 
    subprojects/imgui/imgui_demo.cpp
    subprojects/imgui/imgui_draw.cpp
    subprojects/imgui/imgui_tables.cpp
    subprojects/imgui/imgui_widgets.cpp
    subprojects/imgui/imgui.cpp
    subprojects/imgui/backends/imgui_impl_glfw.cpp
    subprojects/imgui/backends/imgui_impl_opengl3.cpp
    )

add_executable(
    ${target_name} 
    PretextView.cpp )
target_compile_definitions(${target_name} PRIVATE PV=${PROJECT_VERSION})
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${target_name} PRIVATE DEBUG)
    target_compile_definitions(copy_texture PRIVATE DEBUG)
endif()
target_link_libraries(${target_name} PRIVATE glfw libdeflate_static ${TORCH_LIBRARIES} aisort glm glad utilsPretextView copy_texture imgui ${OPENBLAS_LIB})   #  Eigen3::Eigen
# 先添加这些库才能添加glfw静态库 （如果是添加动态库无所谓）
# 定义需要的库列表，寻找这些库并且将这些库添加到make的目标中
set(library_need Cocoa OpenGL IOKit CoreVideo) # 库名完全小写也可以，cmake文件的命令可以全部小写，目前看来是这样
if (APPLE)
    foreach (library ${library_need})
        find_library(${library}_LIB ${library})
        if (NOT ${library}_LIB)
            message(FATAL_ERROR "${library} not found")
        else()
            message(STATUS "${library} found: ${${library}_LIB}")  
        endif()
        target_link_libraries(${target_name} PRIVATE ${${library}_LIB} )
    endforeach()
endif()


# set the shared/dynamic library path for the installed binary
set_target_properties(
    ${target_name} 
    PROPERTIES 
    INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_SOURCE_DIR}/subprojects/libtorch/lib")  
install(TARGETS ${target_name} DESTINATION bin)