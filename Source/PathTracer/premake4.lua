
dofile("../../ProjectGen/common.lua")

local SolutionName = "PathTracer"
local Platform = "x64"
local ExtraDefines = { "IsWindows_=1", "Is64Bit_=1", "PathTracer_=1", "FriendlyAppName_=\"PathTracer\"" }
local ExtraLibraries = {}
ExtraLibraries["Core"] = { "SceneLib" }

local ExtraMiddlewareIncludeDirs = { "Embree-3.0/include" }
local ExtraMiddlewareLinkDirs = { "Embree-3.0/lib" }
local ExtraMiddlewareLibraries = { "embree3" }
local ExtraDllCopies = { "Embree-3.0\\bin\\embree3.dll $(TargetDir)embree3.dll", "Embree-3.0\\bin\\tbb.dll $(TargetDir)tbb.dll" }

SetupConsoleApplication(SolutionName, Platform, ExtraDefines, false, ExtraLibraries, ExtraMiddlewareIncludeDirs, ExtraMiddlewareLinkDirs, ExtraMiddlewareLibraries, ExtraDllCopies)