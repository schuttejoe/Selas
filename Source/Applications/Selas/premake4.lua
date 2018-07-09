
dofile("../../../ProjectGen/common.lua")

local SolutionName = "Selas"
local Architecture = "x64"
local ExtraLibraries = { "SceneLib", "TextureLib", "GeometryLib", "Bsdf", "Shading", "BuildCore", "BuildCommon" }

if _ARGS[1] == "osx" then
	ExtraDefines = { "IsOsx_=1" }
	Platform = "osx"
else
	ExtraDefines = { "IsWindows_=1" }
	Platform = "Win64"
end

SetupConsoleApplication(SolutionName, Architecture, Platform, ExtraDefines, ExtraLibraries)