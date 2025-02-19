cmake_minimum_required(VERSION 3.17)
project(PretextView VERSION "1.0.0")


# 新增编译选项 --------------------------------------------------
option(FORCE_MAC_X86 "Force macOS x86_64 build" OFF)  
option(BUILD_UNIVERSAL "Build macOS universal binary" OFF) 
option(WITH_TORCH "Find and link torch lib" OFF)

set(CMAKE_C_STANDARD 17)   
set(CMAKE_CXX_STANDARD 17)   
set(CMAKE_C_STANDARD_REQUIRED ON)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  

set(target_name PretextViewAI)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)

    if(FORCE_MAC_X86) 
        message(STATUS "Forcing x86_64 architecture build")
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architectures" FORCE)
    elseif(BUILD_UNIVERSAL) 
        message(STATUS "Building macOS universal binary (x86_64 + arm64)")
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures" FORCE)
    else()  
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
            set(CMAKE_OSX_ARCHITECTURES "arm64")
            message(STATUS "Auto-detected architecture: arm64")
        else()
            set(CMAKE_OSX_ARCHITECTURES "x86_64")
            message(STATUS "Auto-detected architecture: x86_64")
        endif()
    endif()

    message(STATUS "MacOS deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
    message(STATUS "MacOS target architectures: ${CMAKE_OSX_ARCHITECTURES}")

elseif(WIN32)
    message(STATUS "Windows build")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Linux build")
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


find_library(
    lib_deflate_static 
    NAMES deflate deflatestatic
    PATHS 
        subprojects/libdeflate/build 
        subprojects/libdeflate/build/debug 
        subprojects/libdeflate/build/Release
        subprojects/libdeflate/build/release)
if(NOT lib_deflate_static)
    message(FATAL_ERROR "lib_deflate_static not found")
else()
    message(STATUS "lib_deflate_static found: ${lib_deflate_static}")
endif()


# set(CMAKE_PREFIX_PATH "subprojects/libtorch/share/cmake/Torch")
if (WITH_TORCH)
    message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
    find_package(Torch REQUIRED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
    message(STATUS "Torch libs: ${TORCH_LIBRARIES}")
    message(STATUS "Torch install prefix: ${TORCH_INSTALL_PREFIX}")
    # set(CMAKE_PREFIX_PATH "../libtorch/share/cmake/Torch")
    set(WITH_PYTHON OFF CACHE BOOL "Disable Python integration for pytorch_scatter" FORCE)
    add_subdirectory(subprojects/pytorch_scatter)
    add_library(sortlib STATIC src/aisort.cpp )
    target_link_libraries(
        sortlib PUBLIC 
        ${TORCH_LIBRARIES} 
        ${torchscatter})
else()
    message(STATUS "Libtorch: Build without libtorch")
    add_library(sortlib STATIC src/frag_sort.cpp )
endif()


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
    sortlib 
    glm 
    glad 
    utilsPretextView 
    copy_texture 
    imgui 
    glfw 
)   

if (APPLE)
    set(library_need Cocoa OpenGL IOKit CoreVideo) 
    # find_library(METAL_LIB Metal)
    # find_library(ACCELERATE_LIB Accelerate)
    # if (METAL_LIB) 
    #     message(STATUS "Metal found1: ${METAL_LIB}")
    #     list(APPEND library_need Metal)
    # endif()
    # if (ACCELERATE_LIB)
    #     message(STATUS "ACCELERATE_LIB found1: ${ACCELERATE_LIB}")
    #     list(APPEND library_need Accelerate)
    # endif()
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
file(GLOB aimodel_files "${CMAKE_CURRENT_SOURCE_DIR}/ai_model/*")
file(GLOB torch_copy_libs "${TORCH_INSTALL_PREFIX}/lib/*")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug on ${CMAKE_SYSTEM_NAME} - ${CMAKE_OSX_ARCHITECTURES}.")
    if (APPLE)
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist" DESTINATION "${target_name}.app/Contents")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/icon_mac.icns" DESTINATION "${target_name}.app/Contents/Resources" )
        if (WITH_TORCH)
            file(COPY ${aimodel_files} DESTINATION "${target_name}.app/Contents/Resources/ai_model")
        endif()
    endif()
endif()

if(APPLE)
    # set the shared/dynamic library path for the installed binary
    message(STATUS "[Cmake Install]: Mac")
    set_target_properties(
        ${target_name} 
        PROPERTIES 
        INSTALL_RPATH "@executable_path/../Frameworks")
    install(TARGETS ${target_name}
            BUNDLE DESTINATION .
            COMPONENT Runtime)   # generate the ${target_name}.app package under ${cmake_install_prefix}
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
            DESTINATION "${target_name}.app/Contents")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icon_mac.icns"
            DESTINATION "${target_name}.app/Contents/Resources" )
    if (WITH_TORCH)
        install(FILES ${torch_copy_libs}
                DESTINATION "${target_name}.app/Contents/Frameworks" )
        install(FILES ${aimodel_files}
                DESTINATION "${target_name}.app/Contents/Resources/ai_model" )
    endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux") 
    message(STATUS "[Cmake Install]: Linux")
    set_target_properties(
        ${target_name}
        PROPERTIES 
        INSTALL_RPATH "$ORIGIN/../lib")
    install(
        TARGETS ${target_name}
        RUNTIME DESTINATION "bin"
        COMPONENT Runtime)
    if (WITH_TORCH)
        install(
            FILES ${torch_copy_libs} 
            DESTINATION "lib") 
        install(
            FILES ${aimodel_files} 
            DESTINATION "ai_model")
    endif()
else () # windows
    message(STATUS "[Cmake Install]: Windows")
    target_sources(${target_name} PRIVATE "PretextView.rc") # add logo to win version
    install(
        TARGETS ${target_name}
        RUNTIME DESTINATION .
        COMPONENT Runtime)
    if (WITH_TORCH)
        install(
            FILES ${torch_copy_libs} 
            DESTINATION .) # libtorch
        install(
            FILES ${aimodel_files}   
            DESTINATION "ai_model")
    endif()
endif()


# ==== CPack: installation package ====
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME ${target_name})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/guanshaoheng/PretextView")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENCE.txt")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin") # macOS
    message(STATUS "[CPack]: Generate on Mac")
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "${target_name} Installer")
    set(CPACK_DMG_FORMAT "UDZO") 
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux") # Linux (Ubuntu)
    message(STATUS "[CPack]: Generate on Linux")
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_HARDLINKS OFF)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Shaoheng Guan") 
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "This is ${target_name}, HiC map viewer, editor with AI-powered automatic curation.")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.29)")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils") 
elseif(WIN32) # Windows
    message(STATUS "[CPack]: Generate on Windows")
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "${target_name}")
    set(CPACK_NSIS_PACKAGE_NAME "${target_name} Installer")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/icon_win.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/icon_win.ico") 
    set(CPACK_NSIS_INSTALLED_ICON_NAME "${target_name}.exe")
    set(CPACK_NSIS_HELP_LINK "https://github.com/guanshaoheng/PretextView") 
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/guanshaoheng/PretextView") 
endif()

include(CPack)