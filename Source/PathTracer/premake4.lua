
dofile("../../ProjectGen/common.lua")

local SolutionName = "PathTracer"
local Platform = "x64"
local ExtraDefines = { "IsWindows_=1", "Is64Bit_=1", "PathTracer_=1", "FriendlyAppName_=\"PathTracer\"" }
local ExtraLibraries = {}
ExtraLibraries["Core"] = { "SceneLib" }

local ExtraMiddlewareIncludeDirs = { "Embree-2.7.1/x64/include" }
local ExtraMiddlewareLinkDirs = { "Embree-2.7.1/x64/lib" }
local ExtraMiddlewareLibraries = { "embree" }
local ExtraDllCopies = { "Embree-2.7.1\\x64\\lib\\embree.dll $(TargetDir)embree.dll", "Embree-2.7.1\\x64\\lib\\tbb.dll $(TargetDir)tbb.dll" }

SetupConsoleApplication(SolutionName, Platform, ExtraDefines, false, ExtraLibraries, ExtraMiddlewareIncludeDirs, ExtraMiddlewareLinkDirs, ExtraMiddlewareLibraries, ExtraDllCopies)