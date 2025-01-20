cmake_minimum_required(VERSION 3.17)
project(PretextView VERSION "1.3.0")

set(CMAKE_C_STANDARD 17)   
set(CMAKE_CXX_STANDARD 17)   
set(CMAKE_C_STANDARD_REQUIRED ON)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  

set(target_name PretextViewAI)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE) # required by libtorch
    # define architecture-specific flags
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(CMAKE_OSX_ARCHITECTURES "arm64")
        message(STATUS "Arch: ${CMAKE_SYSTEM_PROCESSOR}.")
    else()
        set(CMAKE_OSX_ARCHITECTURES "x86_64")
        message(STATUS "Arch: ${CMAKE_SYSTEM_PROCESSOR}.")
    endif()
    message(STATUS "macOS version: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
elseif(WIN32)
    message(STATUS "Windows")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Linux")
endif()

message(STATUS "Install path: ${CMAKE_INSTALL_PREFIX}")


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
)

add_subdirectory(subprojects/glfw)
add_subdirectory(subprojects/glm)

find_library(lib_deflate_static deflate PATHS subprojects/libdeflate/build)
if(NOT lib_deflate_static)
    message(FATAL_ERROR "lib_deflate_static not found")
else()
    message(STATUS "lib_deflate_static found: ${lib_deflate_static}")
endif()


# if (WITH_PYTHON)
#   add_definitions(-DWITH_PYTHON)
#   find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
#   message(STATUS "Python found: ${Python3_EXECUTABLE}")
#   execute_process(
#     COMMAND "${Python3_EXECUTABLE}" -c "import torch; print(torch.__path__[0])"
#     OUTPUT_VARIABLE TORCH_PYTHON_PATH
#     OUTPUT_STRIP_TRAILING_WHITESPACE
#   )
#   if (TORCH_PYTHON_PATH)
#     message(STATUS "Torch installed in Python at: ${TORCH_PYTHON_PATH}")
#   else()
#     message(FATAL_ERROR "Torch not found in Python")
#   endif()
#   list(APPEND CMAKE_PREFIX_PATH "${TORCH_PYTHON_PATH}/share/cmake/Torch")
# endif()

# set(CMAKE_PREFIX_PATH "subprojects/libtorch/share/cmake/Torch")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
message(STATUS "Torch libs: ${TORCH_LIBRARIES}")
message(STATUS "Torch install prefix: ${TORCH_INSTALL_PREFIX}")
# set(CMAKE_PREFIX_PATH "../libtorch/share/cmake/Torch")
set(WITH_PYTHON OFF CACHE BOOL "Disable Python integration for pytorch_scatter" FORCE)
add_subdirectory(subprojects/pytorch_scatter)


# OpenBLAS, which was required before, now is not used
# include_directories(
#     subprojects/OpenBLAS
#     subprojects/OpenBLAS/build
# )
# find_library(OPENBLAS_LIB openblas PATHS subprojects/OpenBLAS/build/lib)
# if(NOT OPENBLAS_LIB)
#     message(FATAL_ERROR "OpenBLAS not found")
# else()
#     message(STATUS "OpenBLAS found: ${OPENBLAS_LIB}")
# endif()


add_library(aisort STATIC src/aisort.cpp )
target_link_libraries(
    aisort PRIVATE 
    ${TORCH_LIBRARIES} 
    ${torchscatter})


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

if(NOT APPLE)
    add_executable( ${target_name} PretextView.cpp )
else()
    add_executable( ${target_name} MACOSX_BUNDLE PretextView.cpp )
endif()
target_compile_definitions(${target_name} PRIVATE PV=${PROJECT_VERSION})
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${target_name} PRIVATE DEBUG)
    target_compile_definitions(copy_texture PRIVATE DEBUG)
    message(STATUS "Debug version: flag (${CMAKE_BUILD_TYPE}) with DEBUG")
else()
    message(STATUS "Release version: flag (${CMAKE_BUILD_TYPE}) without DEBUG")
endif()
target_link_libraries(
    ${target_name} PRIVATE 
    ${lib_deflate_static} 
    ${TORCH_LIBRARIES} 
    aisort 
    glm 
    glad 
    utilsPretextView 
    copy_texture 
    imgui 
    glfw 
)   


if (APPLE)
    set(library_need Cocoa OpenGL IOKit CoreVideo) 
elseif(WIN32)
    set(library_need gdi32 opengl32 kernel32 user32 shell32 Ole32) 
elseif(UNIX AND NOT APPLE)
    set(library_need m dl X11) 
else()
    message(FATAL_ERROR "Unsupported platform")
endif()


foreach (library ${library_need})
    if ( NOT WIN32)
        find_library(${library}_LIB ${library})
        if (NOT ${library}_LIB)
            message(FATAL_ERROR "${library} not found")
        else()
            message(STATUS "${library} found: ${${library}_LIB}")  
        endif()
        target_link_libraries(${target_name} PRIVATE ${${library}_LIB} )
    else() # WIN32
        foreach (library ${library_need})
            target_link_libraries(${target_name} PRIVATE ${library} )
        endforeach()
    endif()
endforeach()


# ============ CMAKE INSTALLATION ==============
# Define shader files
file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/shaderSource/*.shader")
file(GLOB aimodel_files "${CMAKE_CURRENT_SOURCE_DIR}/ai_model/*")
file(GLOB torch_copy_libs "${TORCH_INSTALL_PREFIX}/lib/*")
# file(COPY ${SHADER_FILES} DESTINATION "${CMAKE_INSTALL_PREFIX}/src/shaderSource")
# file(COPY ${aimodel_files} DESTINATION "${CMAKE_INSTALL_PREFIX}/ai_model")

# Define installation paths for macOS
# CMAKE_INSTALL_PREFIX = PretextView.app
if(APPLE)
    # set the shared/dynamic library path for the installed binary
    set_target_properties(
        ${target_name} 
        PROPERTIES 
        INSTALL_RPATH "@executable_path/../Frameworks")
    install(TARGETS ${target_name}
            BUNDLE DESTINATION .
            COMPONENT Runtime)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
            DESTINATION "${target_name}.app/Contents")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icon.icns"
            DESTINATION "${target_name}.app/Contents/Resources" )
    install(FILES ${torch_copy_libs}
            DESTINATION "${target_name}.app/Contents/Frameworks" )
    install(FILES ${aimodel_files}
            DESTINATION "${target_name}.app/Contents/Resources/ai_model" )
    install(FILES ${SHADER_FILES}
            DESTINATION "${target_name}.app/Contents/Resources/src/shaderSource" )

else() # WIN32, UNIX and NOT APPLE
    set_target_properties(
            ${target_name} 
            PROPERTIES 
            INSTALL_RPATH "@executable_path/lib")
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        set(app_dir "${CMAKE_INSTALL_PREFIX}/${target_name}.linux")
    else()
        set(app_dir "${CMAKE_INSTALL_PREFIX}/${target_name}.windows")
    endif()
    install(
        TARGETS ${target_name} 
        RUNTIME DESTINATION "${app_dir}"
        COMPONENT Runtime)
    install(FILES ${torch_copy_libs} DESTINATION "${app_dir}/lib") # libtorch
    install(FILES ${SHADER_FILES} DESTINATION "${app_dir}/src/shaderSource")
    install(FILES ${aimodel_files} DESTINATION "${app_dir}/ai_model")
endif()


# ==== CPack: installation package ====
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME ${target_name})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/guanshaoheng/PretextView")
set(CPACK_PACKAGE_ICON ${CMAKE_CURRENT_SOURCE_DIR}/icon.icns)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENCE.txt")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin") # macOS
    message(STATUS "[CPack]: Generate on MacOS")
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "${target_name} Installer")
    set(CPACK_DMG_FORMAT "UDZO") 
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux") # Linux (Ubuntu)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_HARDLINKS OFF)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Shaoheng Guan") 
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "This is ${target_name}, HiC map viewer, editor with AI-powered automatic curation.")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.29)")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils") 
elseif(WIN32) # Windows
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "${target_name}")
    set(CPACK_NSIS_PACKAGE_NAME "${target_name} Installer")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/icon.icns") 
    set(CPACK_NSIS_HELP_LINK "https://github.com/guanshaoheng/PretextView") 
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/guanshaoheng/PretextView") 
endif()
include(CPack)