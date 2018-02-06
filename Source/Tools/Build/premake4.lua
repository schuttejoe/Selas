

dofile("../../../ProjectGen/common.lua")

local SolutionName = "Build"
local Platform = "x64"
local ExtraDefines = { "IsWindows_=1", "Is64Bit_=1", "Build_=1", "FriendlyAppName_=\"Build\"" }
local ExtraLibraries = {}
ExtraLibraries["Tool"] = { "BuildCommon" }

local ExtraMiddlewareIncludeDirs = { }
local ExtraMiddlewareLinkDirs = { }
local ExtraMiddlewareLibraries = { "assimp" }
local ExtraDllCopies = { "$(TargetDir)assimp.dll" }

SetupConsoleApplication(SolutionName, Platform, ExtraDefines, ExtraLibraries, ExtraMiddlewareIncludeDirs, ExtraMiddlewareLinkDirs, ExtraMiddlewareLibraries, ExtraDllCopies)