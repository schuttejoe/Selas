
local platform = ...

if platform == "Win64" then
    includedirs { MiddlewareDir .. "Embree-latest/include" }
    table.insert(middlewareLinkDirectories, "Embree-latest/lib")
    table.insert(middlewareLibraries, "embree3")
    table.insert(postBuildCopies, "Embree-latest\\lib\\embree3.dll $(TargetDir)embree3.dll")
elseif platform == "osx" then
    includedirs { MiddlewareDir .. "embree-3.2.0-osx/include" }

    table.insert(middlewareLinkDirectories, "embree-3.2.0-osx/lib")
    table.insert(middlewareLibraries, "libembree3")
    -- table.insert(postBuildCopies, "embree-3.2.0-osx/lib/libembree3.dylib $(TargetDir)libembree3.dylib")
    -- table.insert(postBuildCopies, "embree-3.2.0-osx/lib/libtbb.dylib $(TargetDir)libtbb.dylib")
end