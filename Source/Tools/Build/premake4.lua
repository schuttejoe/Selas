

dofile("../../../ProjectGen/common.lua")

local SolutionName = "Build"
local Platform = "x64"
local ExtraDefines = { "IsWindows_=1", "Is64Bit_=1", "Build_=1", "FriendlyAppName_=\"Build\"" }
local ExtraLibraries = {}
ExtraLibraries["Tool"] = { "BuildCommon" }

local ExtraMiddlewareIncludeDirs = { "assimp-4.1.0\\include" }
local ExtraMiddlewareLinkDirs = { "assimp-4.1.0\\lib\\Release"}
local ExtraMiddlewareLibraries = { "assimp-vc140-mt" }
local ExtraDllCopies = { "assimp-4.1.0\\bin\\assimp-vc140-mt.dll $(TargetDir)assimp-vc140-mt.dll" }

SetupConsoleApplication(SolutionName, Platform, ExtraDefines, true, ExtraLibraries, ExtraMiddlewareIncludeDirs, ExtraMiddlewareLinkDirs, ExtraMiddlewareLibraries, ExtraDllCopies)