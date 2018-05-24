
dofile("../../../ProjectGen/common.lua")

local SolutionName = "Build"
local Architecture = "x64"
local ExtraDefines = { "IsWindows_=1" }
local ExtraLibraries = { "SceneLib", "TextureLib", "GeometryLib", "BuildCommon" }

SetupConsoleApplication(SolutionName, Architecture, ExtraDefines, ExtraLibraries)