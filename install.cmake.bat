@ECHO OFF


REM Expecting the Torch path as the first argument
set "python_path=%~1"
echo Python path received: %python_path%
REM ... use %TORCH_PATH% as needed ...


SET CC=clang-cl
SET CXX=clang-cl
SET WINDRES=rc


REM ========= pull git repo =========
git submodule update --init --recursive


@REM REM ========= libtorch =========
@REM set "LIBTORCH_URL=https://download.pytorch.org/libtorch/cpu/libtorch-win-shared-with-deps-2.5.1%%2Bcpu.zip"
@REM set "libtorch_zip_file=libtorch.zip"
@REM set "DEST_DIR=subprojects"

@REM REM Create the destination directory if it does not exist
@REM if not exist "%DEST_DIR%" (
@REM     mkdir "%DEST_DIR%"
@REM )

@REM REM Check if the libtorch folder exists in DEST_DIR
@REM if not exist "%DEST_DIR%\libtorch" (
@REM     REM Check if libtorch.zip exists
@REM     if not exist "%libtorch_zip_file%" (
@REM         echo libtorch.zip not found. Downloading...
@REM         curl -L -o "%libtorch_zip_file%" "%LIBTORCH_URL%"
@REM     ) else (
@REM         echo %libtorch_zip_file% already exists. Skipping download.
@REM     )
@REM     echo Extracting libtorch.zip to %DEST_DIR%\libtorch...
@REM     REM Extract using PowerShell Expand-Archive (force overwrite if needed)
@REM     start /wait powershell -NoLogo -NoProfile -ExecutionPolicy Bypass -Command "Expand-Archive -Force -Path '%libtorch_zip_file%' -DestinationPath '%DEST_DIR%'" >NUL 2>&1
@REM     echo Extraction completed.
@REM ) else (
@REM     echo %DEST_DIR%\libtorch already exists. Skipping extraction.
@REM )

@REM if exist "%DEST_DIR%\libtorch\share\cmake\Torch\TorchConfig.cmake" (
@REM     echo libtorch successfully installed.
@REM ) else (
@REM     echo "[fatal error]: no %DEST_DIR%\libtorch\share\cmake\Torch\TorchConfig.cmake"
@REM     goto :error
@REM )

@REM if exist "%DEST_DIR%\libtorch\include\ATen\OpMathType.h" (
@REM     echo "%DEST_DIR%\libtorch\include\ATen\OpMathType.h found"
@REM ) else (
@REM     echo "Fatal error: %DEST_DIR%\libtorch\include\ATen\OpMathType.h not found."
@REM     goto :error
@REM )

@REM REM Clean up the zip file if it exists
@REM if exist "%libtorch_zip_file%" (
@REM     del "%libtorch_zip_file%"
@REM )

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

cmake -DCMAKE_BUILD_TYPE=Release -DGLFW_USE_WAYLAND=OFF -DGLFW_BUILD_X11=OFF -DWITH_PYTHON=OFF -DCMAKE_INSTALL_PREFIX=PretextViewAI.windows -DCMAKE_PREFIX_PATH=%python_path% -S . -B build_cmake
if errorlevel 1 (
    echo "CMake configuration failed."
    goto :error
)
else (
    echo "CMake configuration completed successfully."
)

cmake --build build_cmake -j 8
if errorlevel 1 (
    echo "Build failed."
    goto :error
)
else (
    echo "Build completed successfully."
)

cmake --install build_cmake
if errorlevel 1 (
    echo "Install failed."
    goto :error
)
else (
    echo "Install completed successfully."
)

echo Build and installation completed successfully.
goto :eof

:error
echo An error occurred during the build process.
exit /b 1
goto :eof
