
local platform = ...

if platform == "Win64" then
    includedirs { MiddlewareDir .. "tbb\\include" }
    table.insert(middlewareLinkDirectories, "tbb\\lib")
    table.insert(middlewareLibraries, "tbb")
    table.insert(postBuildCopies, "tbb\\bin\\tbb.dll $(TargetDir)tbb.dll")
    table.insert(postBuildCopies, "tbb\\bin\\tbbmalloc.dll $(TargetDir)tbbmalloc.dll")
end
