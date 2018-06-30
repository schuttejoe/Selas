
local platform = ...

if platform == "Win64" and LinkWinPixRuntime then
	includedirs { MiddlewareDir .. "WinPixEventRuntime\\include" }

	table.insert(middlewareLinkDirectories, "WinPixEventRuntime\\bin")
	table.insert(middlewareLibraries, "WinPixEventRuntime")
	table.insert(postBuildCopies, "WinPixEventRuntime\\bin\\WinPixEventRuntime.dll $(TargetDir)WinPixEventRuntime.dll")
end