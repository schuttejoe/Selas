
echo off
pushd "%ShootyEngine%"

echo "Generating Solutions..."

cd "Source\Applications\PathTracer"
call "MakeSln.bat"

cd "Source\Applications\Build"
call "MakeSln.bat"

popd