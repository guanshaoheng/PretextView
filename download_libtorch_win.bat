@ECHO OFF

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