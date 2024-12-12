cmake_minimum_required(VERSION 3.17)
project(PretextView VERSION "1.0.0")


set(target_name PretextViewAI)


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


set(CMAKE_C_STANDARD 17)   # 设置cpp版本
set(CMAKE_CXX_STANDARD 17)   # 设置cpp版本
set(CMAKE_C_STANDARD_REQUIRED ON)  # 设置cpp版本
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # 设置cpp版本

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

set(CMAKE_PREFIX_PATH "subprojects/libtorch/share/cmake/Torch")
add_subdirectory(subprojects/pytorch_scatter)

set(CMAKE_PREFIX_PATH "subprojects/libtorch/share/cmake/Torch")
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
message(STATUS "Torch found: ${TORCH_LIBRARIES}")


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


# find_library(PYTORCH_EXTENSION_LIBRARY
#     NAMES libtorchscatter.dylib
#     PATHS subprojects/pytorch_scatter/build
#     REQUIRED)
# message(STATUS "Pytorch scatter found: ${PYTORCH_EXTENSION_LIBRARY}")


add_library(aisort STATIC src/aisort.cpp )
target_link_libraries(aisort PRIVATE ${TORCH_LIBRARIES} ${torchscatter})


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
    message(STATUS "Debug version: flag (${CMAKE_BUILD_TYPE}) with DEBUG")
else()
    message(STATUS "Release version: flag (${CMAKE_BUILD_TYPE}) without DEBUG")
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
    INSTALL_RPATH "@executable_path/../../lib")  

# Define shader files
set(SHADER_FILES
    src/shaderSource/contactMatrixFragment.shader
    src/shaderSource/contactMatrixFragmentHIC.shader
    src/shaderSource/contactMatrixVertex.shader
    src/shaderSource/contactMatrixVertexHIC.shader
    src/shaderSource/editFragment.shader
    src/shaderSource/editGeometry.shader
    src/shaderSource/editVertex.shader
    src/shaderSource/flatFragment.shader
    src/shaderSource/flatVertex.shader
    src/shaderSource/uiFragment.shader
    src/shaderSource/uiVertex.shader
)

file( COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/shaderSource DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/src )


file(GLOB TORCH_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libtorch/lib/*.dylib")
file( COPY ${TORCH_LIBRARIES} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib )

# Define installation paths for macOS
# CMAKE_INSTALL_PREFIX = PretextView.app
if(APPLE)
    set(APPS "${CMAKE_INSTALL_PREFIX}/Contents")
    set(EXECUTABLE_DIR "${APPS}/MacOS")
    set(RESOURCES_DIR "${APPS}/Resources")
    
    install(FILES ${SHADER_FILES} DESTINATION "${RESOURCES_DIR}/src/shaderSource") # Install shader files to Resources
    install(FILES Info.plist DESTINATION "${APPS}")  # Install Info.plist
    install(FILES icon.icns DESTINATION "${RESOURCES_DIR}")  # Install icon.icns
    install(FILES subprojects/libtorch/lib/libtorch.dylib DESTINATION "${CMAKE_INSTALL_PREFIX}/lib") # Install the libtorch library
    install(TARGETS torchscatter DESTINATION "${CMAKE_INSTALL_PREFIX}/lib") # Install the torchscatter library
    install(TARGETS ${target_name} DESTINATION "${EXECUTABLE_DIR}") # Install the executable


    # todo (shaoheng) copy ai model into 
endif()