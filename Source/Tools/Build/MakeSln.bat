@echo off

echo.
echo "Generating Win64 Build..."
rd /s /q ..\..\..\_Projects\Build
call ..\..\..\Middleware\Premake\premake5.exe vs2017 win64

@echo on
