cmake_minimum_required(VERSION 3.17)
project(PretextView VERSION "0.2.5")

set(target_name PretextView)

set(CMAKE_C_STANDARD 17)   # 设置cpp版本
set(CMAKE_CXX_STANDARD 17)   # 设置cpp版本
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")  
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_BUILD_TYPE Debug)  # 设置调试版本


set(source_file PretextView.cpp glad.c)
include_directories(
    include
    src
    subprojects/glfw/include
    subprojects/libdeflate
)


add_subdirectory(subprojects/glfw)
add_subdirectory(subprojects/libdeflate)

set(CMAKE_PREFIX_PATH "subprojects/libtorch/share/cmake/Torch")
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

add_library(aisort STATIC src/aisort.cpp )
target_link_libraries(aisort PRIVATE ${TORCH_LIBRARIES})

add_executable(
    ${target_name} 
    ${source_file} 
    )

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

target_link_libraries(${target_name} PRIVATE glfw libdeflate_static "${TORCH_LIBRARIES}" aisort)  


# set the shared/dynamic library path for the installed binary
set_target_properties(
    ${target_name} 
    PROPERTIES 
    INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_SOURCE_DIR}/subprojects/libtorch/lib")  
install(TARGETS ${target_name} DESTINATION bin)