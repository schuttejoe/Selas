
dofile("../../../ProjectGen/common.lua")

local SolutionName = "Build"
local Architecture = "x64"
local ExtraLibraries = { "SceneLib", "TextureLib", "GeometryLib", "BuildCore", "BuildCommon" }

if _ARGS[1] == "linux64" then
	ExtraDefines = { "IsLinux_=1" }
else
	ExtraDefines = { "IsWindows_=1" }
end

SetupConsoleApplication(SolutionName, Architecture, ExtraDefines, ExtraLibraries)