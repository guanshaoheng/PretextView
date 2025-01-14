@ECHO OFF


SET CC=clang-cl
SET CXX=clang-cl
SET WINDRES=rc


REM ========= pull git repo =========
git submodule update --init --recursive


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
        curl -o "%libtorch_zip_file%" "%LIBTORCH_URL%"
    ) else (
        echo %libtorch_zip_file% already exists. Skipping download.
    )
    echo Extracting libtorch.zip to %DEST_DIR%\libtorch...
    REM Extract using PowerShell Expand-Archive (force overwrite if needed)
    start /wait powershell -NoLogo -NoProfile -ExecutionPolicy Bypass -Command "Expand-Archive -Force -Path '%libtorch_zip_file%' -DestinationPath '%DEST_DIR%'" >NUL 2>&1
    echo Extraction completed.
) else (
    echo %DEST_DIR%\libtorch already exists. Skipping extraction.
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


REM ========= pytorch_scatter =========
cd .\subprojects\pytorch_scatter
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=..\libtorch\share\cmake\Torch  -S . -B build && cmake --build build -j 8 
if errorlevel 1 (
    echo "[Failed]: Compile pytorch_scatter."
    goto :error
)
else (
    echo "[Successed]: Compile pytorch_scatter."
)
cd ..\..


REM ========= Build the project =========
if exist build_cmake (
    rmdir /s /q build_cmake
    echo "Removed existing build directory."
)

cmake -DCMAKE_BUILD_TYPE=Release -DGLFW_USE_WAYLAND=OFF -DGLFW_BUILD_X11=OFF -DCMAKE_INSTALL_PREFIX=PretextViewAI.windows -S . -B build_cmake
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
