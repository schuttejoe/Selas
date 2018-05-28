@echo off

echo.
echo "Generating Win64 Selas..."
rd /s /q ..\..\..\_Projects\Selas
call ..\..\..\Middleware\Premake\premake5.exe vs2017 win64

@echo on
