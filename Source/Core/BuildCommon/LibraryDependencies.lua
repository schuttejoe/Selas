
local platform = ...

includedirs { MiddlewareDir .. "assimp-4.1.0\\include" }
includedirs { MiddlewareDir .. "rapidjson\\include" }

table.insert(middlewareLibraries, "libassimp")

if platform == "Win64" then
	table.insert(middlewareLinkDirectories, "assimp-4.1.0\\lib\\Release")
	table.insert(postBuildCopies, "assimp-4.1.0\\bin\\Release\\assimp-vc140-mt.dll $(TargetDir)assimp-vc140-mt.dll")
elseif platform == "osx" then
	table.insert(middlewareLinkDirectories, "assimp-osx\\lib")
end

