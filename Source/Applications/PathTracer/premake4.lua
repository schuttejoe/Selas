
dofile("../../../ProjectGen/common.lua")

local SolutionName = "PathTracer"
local Architecture = "x64"
local ExtraDefines = { "IsWindows_=1" }
local ExtraLibraries = { "SceneLib", "TextureLib", "GeometryLib" }

SetupConsoleApplication(SolutionName, Architecture, ExtraDefines, ExtraLibraries)