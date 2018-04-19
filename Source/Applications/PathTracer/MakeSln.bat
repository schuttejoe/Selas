@echo off

echo.
echo "Generating Win64 PathTracer..."
rd /s /q ..\..\..\_Projects\PathTracer
call ..\..\..\Middleware\Premake\premake5.exe vs2017 win64

@echo on
