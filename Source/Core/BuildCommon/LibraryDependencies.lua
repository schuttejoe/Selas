
local platform = ...

includedirs { MiddlewareDir .. "assimp-latest\\include" }
includedirs { MiddlewareDir .. "rapidjson\\include" }

if platform == "Win64" then
	table.insert(middlewareLinkDirectories, "assimp-latest\\lib\\Release")
	table.insert(postBuildCopies, "assimp-latest\\lib\\Release\\assimp-vc140-mt.dll $(TargetDir)assimp-vc140-mt.dll")
    table.insert(middlewareLibraries, "assimp-vc140-mt")
elseif platform == "osx" then
	table.insert(middlewareLinkDirectories, "assimp-osx\\lib")
    table.insert(middlewareLibraries, "libassimp")
end

