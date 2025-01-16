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
add_subdirectory(subprojects/libdeflate)
add_subdirectory(subprojects/glm)


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
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
message(STATUS "Torch found: ${TORCH_LIBRARIES}")
# set(CMAKE_PREFIX_PATH "../libtorch/share/cmake/Torch")
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
target_link_libraries(
    ${target_name} PRIVATE 
    glfw 
    libdeflate_static 
    ${TORCH_LIBRARIES} 
    aisort 
    glm 
    glad 
    utilsPretextView 
    copy_texture 
    imgui 
    # ${OPENBLAS_LIB} # which is used before
)   #  Eigen3::Eigen


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


# set the shared/dynamic library path for the installed binary
set_target_properties(
    ${target_name} 
    PROPERTIES 
    INSTALL_RPATH "@executable_path/../../lib")  

# Define shader files
file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/shaderSource/*.shader")
file(COPY ${SHADER_FILES} DESTINATION "${CMAKE_INSTALL_PREFIX}/src/shaderSource")
file(GLOB aimodel_files "${CMAKE_CURRENT_SOURCE_DIR}/ai_model/*")
file(COPY ${aimodel_files} DESTINATION "${CMAKE_INSTALL_PREFIX}/ai_model")
file(GLOB TORCH_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libtorch/lib/*.dylib")

# Define installation paths for macOS
# CMAKE_INSTALL_PREFIX = PretextView.app
if(APPLE)
    set(APPS "${CMAKE_INSTALL_PREFIX}/Contents")
    set(EXECUTABLE_DIR "${APPS}/MacOS")
    set(RESOURCES_DIR "${APPS}/Resources")
    
    install(FILES ${SHADER_FILES} DESTINATION "${RESOURCES_DIR}/src/shaderSource") # Install shader files to Resources
    install(FILES Info.plist DESTINATION "${APPS}")  # Install Info.plist
    install(FILES icon.icns DESTINATION "${RESOURCES_DIR}")  # Install icon.icns
    install(FILES  ${SHADER_FILES} DESTINATION "${CMAKE_INSTALL_PREFIX}/src/shaderSource") # Install the shaderSource
    install(FILES ${TORCH_LIBRARIES} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib") # Install the libtorch library
    install(TARGETS ${torchscatter} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib") # Install the torchscatter library
    install(TARGETS ${target_name} DESTINATION "${EXECUTABLE_DIR}") # Install the executable

    # copy ai model into install dir
    install(FILES ${aimodel_files} DESTINATION "${CMAKE_INSTALL_PREFIX}/ai_model")
else() # WIN32, UNIX and NOT APPLE
    install(TARGETS ${target_name} DESTINATION "${CMAKE_INSTALL_PREFIX}")
    install(TARGETS ${torchscatter} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    install(FILES ${TORCH_LIBRARIES} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
    install(FILES ${SHADER_FILES} DESTINATION "${CMAKE_INSTALL_PREFIX}/src/shaderSource")
    install(FILES ${aimodel_files} DESTINATION "${CMAKE_INSTALL_PREFIX}/ai_model")
endif()