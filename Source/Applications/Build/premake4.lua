
dofile("../../../ProjectGen/common.lua")

local SolutionName = "Build"
local Architecture = "x64"
local ExtraDefines = { "IsWindows_=1" }
local ExtraLibraries = { "TextureLib", "GeometryLib", "BuildCommon", "json" }

SetupConsoleApplication(SolutionName, Architecture, ExtraDefines, ExtraLibraries)