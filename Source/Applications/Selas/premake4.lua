
dofile("../../../ProjectGen/common.lua")

local SolutionName = "Selas"
local Architecture = "x64"
local ExtraLibraries = { "SceneLib", "TextureLib", "GeometryLib", "Bsdf", "Shading" }

if _ARGS[1] == "linux64" then
	ExtraDefines = { "IsLinux_=1" }
else
	ExtraDefines = { "IsWindows_=1" }
end

SetupConsoleApplication(SolutionName, Architecture, ExtraDefines, ExtraLibraries)