
dofile("../../../ProjectGen/common.lua")

local SolutionName = "Build"
local Architecture = "x64"
local ExtraLibraries = { "SceneLib", "TextureLib", "GeometryLib", "BuildCore", "BuildCommon" }

if _ARGS[1] == "osx" then
	ExtraDefines = { "IsLinux_=1" }
	Platform = "osx"
else
	ExtraDefines = { "IsWindows_=1" }
	Platform = "Win64"
end

SetupConsoleApplication(SolutionName, Architecture, Platform, ExtraDefines, ExtraLibraries)