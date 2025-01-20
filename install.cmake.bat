@ECHO OFF


REM Expecting the Torch path as the first argument
set "cmake_prefix_path_tmp=subprojects/libtorch/share/cmake"
echo cmake_prefix_path_tmp: %cmake_prefix_path_tmp%
REM ... use %TORCH_PATH% as needed ...


SET CC=clang-cl
SET CXX=clang-cl
SET WINDRES=rc


REM ========= pull git repo =========
git submodule update --init --recursive


REM ========= deflate =========
cd subprojects\libdeflate
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build && cmake --build build --target libdeflate_static
if errorlevel 1 (
    echo "CMake delfate failed."
    goto :error
) else (
    echo "CMake delfate completed successfully."
)
cd ..\..\

REM ========= libtorch =========
set "LIBTORCH_URL=https://download.pytorch.org/libtorch/cpu/libtorch-win-shared-with-deps-2.5.1%%2Bcpu.zip"
set "libtorch_zip_file=libtorch.zip"
set "DEST_DIR=subprojects"

REM Create the destination directory if it does not exist
if not exist "%DEST_DIR%" (
    mkdir "%DEST_DIR%"
)

REM Check if the libtorch folder exists in DEST_DIR
if not exist "%DEST_DIR%\libtorch" (
    REM Check if libtorch.zip exists
    if not exist "%libtorch_zip_file%" (
        echo libtorch.zip not found. Downloading...
        curl -L -o "%libtorch_zip_file%" "%LIBTORCH_URL%"
    ) else (
        echo %libtorch_zip_file% already exists. Skipping download.
    )
    echo Extracting libtorch.zip to %DEST_DIR%\libtorch...
    REM Extract using PowerShell Expand-Archive (force overwrite if needed)
    powershell -Command "Expand-Archive -Path '%libtorch_zip_file%' -DestinationPath '%DEST_DIR%' -Force" >NUL 2>&1
    echo Extraction completed.
) else (
    echo %DEST_DIR%\libtorch already exists. Skipping extraction.
)

if exist "%DEST_DIR%\libtorch\share\cmake\Torch\TorchConfig.cmake" (
    echo libtorch successfully installed.
) else (
    echo "[fatal error]: no %DEST_DIR%\libtorch\share\cmake\Torch\TorchConfig.cmake"
    goto :error
)

if exist "%DEST_DIR%\libtorch\include\ATen\OpMathType.h" (
    echo "%DEST_DIR%\libtorch\include\ATen\OpMathType.h found"
) else (
    echo "Fatal error: %DEST_DIR%\libtorch\include\ATen\OpMathType.h not found."
    goto :error
)

REM Clean up the zip file if it exists
if exist "%libtorch_zip_file%" (
    del "%libtorch_zip_file%"
)

REM ========= blas =========
@REM REM Install OpenBLAS if its build directory does not exist
@REM if not exist "subprojects\OpenBLAS\build" (
@REM     cd subprojects\OpenBLAS
@REM     cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DNOFORTRAN=1
@REM     cmake --build build --config Release -j 8
@REM     if errorlevel 1 (
@REM         echo "OpenBLAS build failed."
@REM         goto :error_blas
@REM     )
@REM     cd ..\..
@REM )


REM ========= Build the project =========
if exist build_cmake (
    rmdir /s /q build_cmake
    echo "Removed existing build directory."
)

if exist app (
    rmdir /s /q app
    echo "Removed existing app directory."
)

cmake -DCMAKE_BUILD_TYPE=Release -DGLFW_BUILD_WAYLAND=OFF -DGLFW_BUILD_X11=OFF -DWITH_PYTHON=OFF -DCMAKE_INSTALL_PREFIX=app -DCMAKE_PREFIX_PATH=%cmake_prefix_path_tmp% -S . -B build_cmake
if errorlevel 1 (
    echo "CMake configuration failed."
    goto :error
) else (
    echo "CMake configuration completed successfully."
)

@REM cmake --build build_cmake --config Release
@REM if errorlevel 1 (
@REM     echo "Build failed."
@REM     goto :error
@REM ) else (
@REM     echo "Build completed successfully."
@REM )

@REM cmake --install build_cmake --config Release
@REM if errorlevel 1 (
@REM     echo "Install failed."
@REM     goto :error
@REM ) else (
@REM     echo "Install completed successfully."
@REM )
@REM echo Build and installation completed successfully.

cmake --build build_cmake --target package
if errorlevel 1 (
    echo "Package failed."
    goto :error
) else (
    echo "Package completed successfully."
)
echo Finished successfully.
goto :eof

:error
echo An error occurred during the build process.
exit /b 1
goto :eof
