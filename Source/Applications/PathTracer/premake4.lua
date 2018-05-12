
dofile("../../../ProjectGen/common.lua")

local SolutionName = "PathTracer"
local Architecture = "x64"
local ExtraDefines = { "IsWindows_=1" }
local ExtraLibraries = { "SceneLib", "TextureLib", "GeometryLib", "Bsdf", "Shading" }

SetupConsoleApplication(SolutionName, Architecture, ExtraDefines, ExtraLibraries)