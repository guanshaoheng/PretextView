cmake_minimum_required(VERSION 3.17)
project(PretextView VERSION "1.0.3")


option(FORCE_MAC_X86 "Force macOS x86_64 build" OFF)
option(BUILD_UNIVERSAL "Build macOS universal binary" OFF) 
option(WITH_TORCH "Find and link torch lib" OFF)
option(DEBUG_OUTPUT_LIKELIHOOD_TABLE "Output likelihood table to file" OFF)
option(DEBUG_OUTPUT_PIXEL_CUT_FILE "Output the pixel cut files" OFF)

set(CMAKE_C_STANDARD 17)   
set(CMAKE_CXX_STANDARD 17)   
set(CMAKE_C_STANDARD_REQUIRED ON)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  

set(target_name PretextViewAI)

if (APPLE)
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

    set(ARCH_TYPE ${CMAKE_OSX_ARCHITECTURES})

else()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
        set(ARCH_TYPE "x86_64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(ARCH_TYPE "ARM64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86")
        set(ARCH_TYPE "x86")
    else()
        message(FATAL_ERROR "Unsupported architecture")
    endif()
    if(WIN32)
        message(STATUS "Windows build")
    elseif(UNIX AND NOT APPLE)
        message(STATUS "Linux build")
    else()  
        message(FATAL_ERROR "Unsupported platform")
    endif()
endif()

if (MSVC)
    add_compile_options(/utf-8)  # force MSVC to use UTF-8 
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

include_directories(
    include
    src
    subprojects/imgui
    subprojects/glfw/include
    subprojects/libdeflate
    subprojects/glm
    subprojects/fmt/include
)

add_subdirectory(subprojects/glfw)
add_subdirectory(subprojects/glm)

find_library(
    lib_fmt
    NAMES fmt libfmt
    PATHS 
        subprojects/fmt/build 
        subprojects/fmt/build/debug 
        subprojects/fmt/build/Release
        subprojects/fmt/build/release)
if(NOT lib_fmt)
    message(FATAL_ERROR "lib_fmt not found")
else()
    message(STATUS "lib_fmt found: ${lib_fmt}")
endif()


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

add_library(lib_yahs STATIC 
    src/yahs_sort.cpp
)

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
        ${torchscatter} 
        lib_yahs)
else()
    message(STATUS "Libtorch: Build without libtorch")
    add_library(sortlib STATIC src/frag_sort.cpp )
    target_link_libraries(sortlib PRIVATE lib_yahs)
endif()


add_library(glad STATIC src/glad.c)
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
    set_target_properties(
        ${target_name} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist"
    )
endif()

target_compile_definitions(${target_name} PRIVATE PV=${PROJECT_VERSION})
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${target_name} PRIVATE DEBUG)
    target_compile_definitions(copy_texture PRIVATE DEBUG)
    message(STATUS "Debug version: flag (${CMAKE_BUILD_TYPE}) with DEBUG")
else()
    message(STATUS "Release version: flag (${CMAKE_BUILD_TYPE}) without DEBUG")
endif()
if (DEBUG_OUTPUT_LIKELIHOOD_TABLE)
    message(STATUS "Debug version: flag (${CMAKE_BUILD_TYPE}) with DEBUG_OUTPUT_LIKELIHOOD_TABLE")
    target_compile_definitions(${target_name} PRIVATE DEBUG_OUTPUT_LIKELIHOOD_TABLE)
endif()

target_link_libraries(
    ${target_name} PRIVATE 
    ${lib_deflate_static} 
    ${lib_fmt}
    sortlib 
    glm 
    glad 
    utilsPretextView 
    copy_texture 
    imgui 
    glfw 
    lib_yahs
)   

if (APPLE)
    set(library_need Cocoa OpenGL IOKit CoreVideo) 
    find_library(METAL_LIB Metal)
    find_library(ACCELERATE_LIB Accelerate)
    if (METAL_LIB) 
        list(APPEND library_need Metal)
    endif()
    if (ACCELERATE_LIB)
        list(APPEND library_need Accelerate)
    endif()
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
if (APPLE)
    execute_process(
        COMMAND bash ${CMAKE_SOURCE_DIR}/make_macos_app_plist.sh ${PROJECT_VERSION}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error_output
        TIMEOUT 30
        )
endif()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug on ${CMAKE_SYSTEM_NAME} - ${ARCH_TYPE}.")
    if (APPLE)
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist" DESTINATION "${target_name}.app/Contents")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/icon.icns" DESTINATION "${target_name}.app/Contents/Resources" )
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
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icon.icns"
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
    target_sources(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/PretextView.rc") # add logo to win version
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
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${ARCH_TYPE}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENCE.txt")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin") # macOS
    message(STATUS "[CPack]: Generate on Mac")
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "${target_name} Installer")
    set(CPACK_DMG_FORMAT "UDZO") 
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux") # Linux (Ubuntu)
    message(STATUS "[CPack]: Generate on Linux")
    install(
        FILES "${CMAKE_SOURCE_DIR}/ico_design/icon_48x48x32.png"
        DESTINATION "/usr/share/icons/hicolor/48x48/apps"
        RENAME "pretextview.png"
    )
    install(
        FILES "${CMAKE_SOURCE_DIR}/ico_design/icon_64x64x32.png"
        DESTINATION "share/icons/hicolor/64x64/apps"
        RENAME "pretextview.png"
    )
    install(
        FILES "${CMAKE_SOURCE_DIR}/ico_design/icon_128x128x32.png"
        DESTINATION "/usr/share/icons/hicolor/128x128/apps"
        RENAME "pretextview.png"
    )
    install(
        FILES "${CMAKE_SOURCE_DIR}/ico_design/icon_256x256x32.png"
        DESTINATION "/usr/share/icons/hicolor/256x256/apps"
        RENAME "pretextview.png"
    )
    install(
        FILES "${CMAKE_SOURCE_DIR}/ico_design/icon_512x512x32.png"
        DESTINATION "/usr/share/icons/hicolor/512x512/apps"
        RENAME "pretextview.png"
    )
    configure_file(
        "${CMAKE_SOURCE_DIR}/pretextviewai.desktop.in"
        "${CMAKE_BINARY_DIR}/pretextviewai.desktop"
        @ONLY
    )
    install(
        FILES "${CMAKE_BINARY_DIR}/pretextviewai.desktop"
        DESTINATION "share/applications"
    )
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
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/pretextview_icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/pretextview_icon.ico") 
    set(CPACK_NSIS_INSTALLED_ICON_NAME "${target_name}.exe")
    set(CPACK_NSIS_HELP_LINK "https://github.com/guanshaoheng/PretextView") 
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/guanshaoheng/PretextView") 
endif()

include(CPack)