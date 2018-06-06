
local platform = ...

includedirs { MiddlewareDir .. "Embree-3.0/include" }

table.insert(middlewareLinkDirectories, "Embree-3.0/lib")
table.insert(middlewareLibraries, "embree3")
table.insert(postBuildCopies, "Embree-3.0\\bin\\embree3.dll $(TargetDir)embree3.dll")
table.insert(postBuildCopies, "Embree-3.0\\bin\\tbb.dll $(TargetDir)tbb.dll")