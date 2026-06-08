@echo off
setlocal enabledelayedexpansion
set "TOOLS_DIR=%~dp0"
set "PROTO_DIR=%TOOLS_DIR%..\port\protobuf_c_port"
set "PATH=%TOOLS_DIR%protoc;%PATH%"

cd /d "%PROTO_DIR%"

if "%~1"=="" (
    for %%f in (*.proto) do (
        echo Generating: %%f
        protoc --c_out=. %%f
    )
) else (
    for %%f in (%*) do (
        echo Generating: %%f
        protoc --c_out=. %%f
    )
)
echo Done.
