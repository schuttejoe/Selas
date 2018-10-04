
local platform = ...

if platform == "Win64" then
    table.insert(middlewareLinkDirectories, "ptex/Release")
    table.insert(middlewareLibraries, "Ptex")
    table.insert(postBuildCopies, "ptex\\Release\\Ptex.dll $(TargetDir)Ptex.dll")
    table.insert(postBuildCopies, "ptex\\Release\\zlib.dll $(TargetDir)zlib.dll")
end 