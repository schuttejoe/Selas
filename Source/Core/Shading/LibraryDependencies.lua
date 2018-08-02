
local platform = ...

if platform == "Win64" then
	includedirs { MiddlewareDir .. "embree-3.0/include" }

	table.insert(middlewareLinkDirectories, "Embree-3.0/lib")
	table.insert(middlewareLibraries, "embree3")
	table.insert(postBuildCopies, "Embree-3.0\\bin\\embree3.dll $(TargetDir)embree3.dll")
	table.insert(postBuildCopies, "Embree-3.0\\bin\\tbb.dll $(TargetDir)tbb.dll")
	
	table.insert(middlewareLinkDirectories, "ptex/Release")
	table.insert(middlewareLibraries, "Ptex")

elseif platform == "osx" then
	includedirs { MiddlewareDir .. "embree-3.2.0-osx/include" }

	table.insert(middlewareLinkDirectories, "embree-3.2.0-osx/lib")
	table.insert(middlewareLibraries, "libembree3")
	-- table.insert(postBuildCopies, "embree-3.2.0-osx/lib/libembree3.dylib $(TargetDir)libembree3.dylib")
	-- table.insert(postBuildCopies, "embree-3.2.0-osx/lib/libtbb.dylib $(TargetDir)libtbb.dylib")
end