
echo off
pushd "%ShootyEngine%"

echo "Generating Solutions..."

cd "Source\PathTracer"
call "MakeSln.bat"

cd "Source\Tools\Build"
call "MakeSln.bat"

popd