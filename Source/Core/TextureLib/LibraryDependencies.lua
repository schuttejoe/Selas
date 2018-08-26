
local platform = ...

includedirs { MiddlewareDir .. "stb" }
if platform == "Win64" then
    table.insert(middlewareLinkDirectories, "ptex/Debug")
    table.insert(middlewareLibraries, "Ptex")
    table.insert(postBuildCopies, "ptex\\Debug\\Ptex.dll $(TargetDir)Ptex.dll")
    table.insert(postBuildCopies, "ptex\\Debug\\zlib.dll $(TargetDir)zlib.dll")
end 