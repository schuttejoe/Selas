
--=================================================================================================
-- Root directory set by running SetEnvironmentVariable in the scripts directory
RootDirectoryName = "Selas"
RootDirectory     = os.getenv(RootDirectoryName)

--=================================================================================================
-- Main directory locations
MiddlewareDir     = RootDirectory .. "Middleware/"
MiddlewareCopyDir = "../../Middleware/"
ProjectGenDir     = RootDirectory .. "ProjectGen/"
SourceDir         = RootDirectory .. "Source/"
CoreDir           = RootDirectory .. "Source/Core/"
BuildTempDir      = RootDirectory .. "_BuildTemp/"
ProjectsTempDir   = RootDirectory .. "_Projects/"

LinkWinPixRuntime = false

--=================================================================================================
-- Oh Lua...
function FileExists(name)
   local f=io.open(name,"r")
   if f~=nil then io.close(f) return true else return false end
end

--=================================================================================================
-- Libraries that are linked by every project
EssentialLibraries = { 
  "ContainersLib",
  "IoLib",
  "MathLib",
  "StringLib",
  "SystemLib",
  "ThreadingLib",
  "UtilityLib",
  "Assets"
}

--=================================================================================================
function CommonSetup (architecture, solutionName, extraDefines)
  -- common include directories
  includedirs { ".", "Source", CoreDir, MiddlewareDir }

  -- common flags
  flags { "ShadowedVariables", "FatalWarnings", "NoIncrementalLink", "StaticRuntime", "No64BitChecks" }
  cppdialect "c++14"
  exceptionhandling ("off")
  rtti ("off")
  symbols "On"
  editandcontinue "Off"

  location(ProjectsTempDir .. solutionName)

  projectRootDef = "ProjectRootName_=\"" .. RootDirectoryName .. "\""

  -- enable multiprocessor builds for MSBuild on windows
  configuration { "windows" }
    flags { "NoMinimalRebuild" }
    buildoptions { "/MP" }

    if LinkWinPixRuntime then
      defines { "USE_PIX" }
    end

  -- common debug defines
  configuration { "Debug" }
    defines { projectRootDef, "Debug_", "_DEBUG=1" }

  -- common release defines
  configuration { "Release" }
    defines { projectRootDef, "Release_" }
    optimize "Full"

    -- common windows defines
  configuration { architecture }
    for i, def in ipairs(extraDefines) do
      defines { def }
    end

  configuration { architecture, "Debug" }
    targetdir(BuildTempDir .. solutionName .. "/" .. architecture .. "/Debug")
    defines { "Debug_=1" }
    targetsuffix("_" .. architecture .. "_dbg")

  configuration { architecture, "Release" }
    targetdir(BuildTempDir .. solutionName .. "/" .. architecture .. "/Release")
    defines { "Release_=1" }
    targetsuffix("_" .. architecture .. "_rel")

  configuration {}
end

--=================================================================================================
function AddStaticLibrary (libraryName, libraryRootPath, solutionName)
  project (libraryName)
    kind "StaticLib"
    location(ProjectsTempDir .. solutionName .. "/" .. libraryName)
    language "C++"
    includedirs { libraryRootPath }
    files { libraryRootPath .. "/**.c", libraryRootPath .. "/**.cpp", libraryRootPath .. "/**.h" }
end

--=================================================================================================
function GatherLibraryDependencies(rootPath, middlewareLinkDirectories, middlewareLibraries, postBuildCopies, platform)
    librarySetupPath = rootPath .. "/LibraryDependencies.lua"
    if FileExists(librarySetupPath) then
      loadfile(librarySetupPath)(platform)
    end
end

--=================================================================================================
function AddCoreLibraries (solutionName, libs, middlewareLinkDirectories, middlewareLibraries, postBuildCopies, platform)
  configuration {}
  for i, lib in ipairs(libs) do
    libraryRootPath = CoreDir .. lib
    AddStaticLibrary(lib, libraryRootPath, solutionName)
    GatherLibraryDependencies(libraryRootPath, middlewareLinkDirectories, middlewareLibraries, postBuildCopies, platform)
  end
end

--=================================================================================================
function LinkLibraries (libs)
  configuration {}
  for i, lib in ipairs(libs) do
    links { lib }
  end
end

--=================================================================================================
function SetupConsoleApplication (solutionName, architecture, platform, extraDefines, extraLibraries)
  solution(solutionName)
    platforms { architecture }
    configurations { "Debug", "Release" }
    CommonSetup(architecture, solutionName, extraDefines)

    middlewareLinkDirectories = {}
    middlewareLibraries = {}
    postBuildCopies = {}

    project "Application"
      kind "ConsoleApp"
      location(ProjectsTempDir .. solutionName)
      targetname(solutionName)
      language "C++"

      files { "Source/**.c", "Source/**.cpp", "Source/**.h" }
      libraryRootPath = "Source/"
      GatherLibraryDependencies(libraryRootPath, middlewareLinkDirectories, middlewareLibraries, postBuildCopies, platform)

      LinkLibraries(EssentialLibraries)
      if extraLibraries ~= nil then
        LinkLibraries(extraLibraries)
      end

    -- LIB PROJECTS
    AddCoreLibraries(solutionName, EssentialLibraries, middlewareLinkDirectories, middlewareLibraries, postBuildCopies, platform)
    if extraLibraries ~= nil then
      AddCoreLibraries(solutionName, extraLibraries, middlewareLinkDirectories, middlewareLibraries, postBuildCopies, platform)
    end
    configuration {}

    project "Application"
      for i, middlewareLinkDir in ipairs(middlewareLinkDirectories) do
        print("Middleware Link Directory: " .. MiddlewareDir .. middlewareLinkDir)
        libdirs { MiddlewareDir .. middlewareLinkDir }
      end

      for i, middlewareLib in ipairs(middlewareLibraries) do
        print("Linking middleware: " .. middlewareLib)
        links { middlewareLib }
      end

      for i, postBuildCopy in ipairs(postBuildCopies) do
        print("Post Build Copy: " .. postBuildCopy)
        postbuildcommands { "copy " .. MiddlewareCopyDir:gsub("/","\\") .. postBuildCopy }
      end
end