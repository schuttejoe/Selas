

dofile("../../../ProjectGen/common.lua")

local SolutionName = "Build"
local Platform = "x64"
local ExtraDefines = { "IsWindows_=1", "Is64Bit_=1", "Build_=1", "FriendlyAppName_=\"Build\"" }
local ExtraLibraries = {}
ExtraLibraries["Tool"] = { "BuildCommon" }

local ExtraMiddlewareIncludeDirs = { "assimp-3.0.1270\\include" }
local ExtraMiddlewareLinkDirs = { "assimp-3.0.1270\\lib\\assimp_release-dll_x64", "assimp-3.0.1270\\lib\\assimp_release-dll_x64"}
local ExtraMiddlewareLibraries = { "assimp" }
local ExtraDllCopies = { "$(TargetDir)assimp.dll" }

SetupConsoleApplication(SolutionName, Platform, ExtraDefines, true, ExtraLibraries, ExtraMiddlewareIncludeDirs, ExtraMiddlewareLinkDirs, ExtraMiddlewareLibraries, ExtraDllCopies)