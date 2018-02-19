
--=================================================================================================
-- Root directory set by running SetEnvironmentVariable in the scripts directory
RootDirectory   = os.getenv("ShootyEngine")

--=================================================================================================
-- Main directory locations
MiddlewareDir     = RootDirectory .. "Middleware/"
MiddlewareCopyDir = "../../Middleware/"
ProjectGenDir     = RootDirectory .. "ProjectGen/"
SourceDir         = RootDirectory .. "Source/"
CoreDir           = RootDirectory .. "Source/Core/"
CoreToolsDir      = RootDirectory .. "Source/CoreTools/"
BuildTempDir      = RootDirectory .. "_BuildTemp/"
ProjectsTempDir   = RootDirectory .. "_Projects/"

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
}

--=================================================================================================
function SetupCommonConfig (ProjectName, ExtraDefines)

  -- common flags
  symbols "On"
  editandcontinue "Off"

  -- enable multiprocessor builds for MSBuild on windows
  configuration { "windows" }
    flags { "NoMinimalRebuild" }
    buildoptions { "/MP" }

  -- common debug defines
  configuration { "Debug" }
    defines { "Debug_", "_DEBUG=1" }
    defines { "AllowDebugTools_=1" }

  -- common dev defines
  configuration { "Profile" }
    defines { "Profile_", "AllowDebugTools_" }
    defines {  }
    optimize "On"

  -- common release defines
  configuration { "Release" }
    defines "Release_"
    optimize "Full"

    -- common windows defines
  configuration { "x64" }
    for i, def in ipairs(ExtraDefines) do
      defines { def }
    end

  configuration { "x64", "Debug" }
    targetdir(BuildTempDir .. ProjectName .. "/x64/Debug")
    defines { "Debug_=1" }
    targetsuffix "_x64_dbg"

  configuration { "x64", "Profile" }
    targetdir(BuildTempDir .. ProjectName .. "/x64/Profile")
    defines { "Profile_=1" }
    targetsuffix "_x64_pro"

  configuration { "x64", "Release" }
    targetdir(BuildTempDir .. ProjectName .. "/x64/Release")
    defines { "Release_=1" }
    targetsuffix "_x64_rel"

  configuration {}
end

--=================================================================================================
function AddLibProjectNoFiles (platformName, projectName, projectPath, solutionName)
  project (projectName)
    kind "StaticLib"
    location(ProjectsTempDir .. solutionName .. "/" .. projectName)
    language "C++"

    includedirs { projectPath }
end

--=================================================================================================
function AddLibProject (platformName, projectName, projectPath, solutionName)
  projectPath = projectPath or "Source/" .. projectName

  AddLibProjectNoFiles (platformName, projectName, projectPath, solutionName)
    files { projectPath .. "/**.c", projectPath .. "/**.cpp", projectPath .. "/**.h" }
end

--=================================================================================================
function AddCoreLibraries (platformName, solutionName, libs)
  configuration {}
  for i, lib in ipairs(libs) do
    AddLibProject (platformName, lib, CoreDir .. lib, solutionName)
    
    -- Need a clean way to define middleware dependencies per library
    if lib == "TextureLib" then
      includedirs { MiddlewareDir .. "stb" }
    end
  end
end

--=================================================================================================
function AddToolLibraries (platformName, solutionName, libs)
  configuration {}
  for i, lib in ipairs(libs) do
    AddLibProject (platformName, lib, CoreToolsDir .. lib, solutionName)
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
function CommonSetup (solutionName, extraDefines, isTool)
  -- common include directories
  includedirs { ".", "Source", CoreDir, MiddlewareDir }
  
  if isTool then
    includedirs { CoreToolsDir }
  end

  -- common flags
  flags { "ShadowedVariables", "FatalWarnings", "NoIncrementalLink", "StaticRuntime", "No64BitChecks" }
  exceptionhandling ("off")
  rtti ("off")

  location(ProjectsTempDir .. solutionName)
  SetupCommonConfig(solutionName, extraDefines)
end

--=================================================================================================
function SetupConsoleApplication (solutionName, platformName, extraDefines, isTool, extraLibraries, extraIncludeDirs, extraMiddlewareLibDirs, extraMiddlewareLibraries, postBuildCopies)
  solution(solutionName)
    platforms { platformName }
    configurations { "Debug", "Profile", "Release" }
    CommonSetup(solutionName, extraDefines, isTool)

    project "Application"
      kind "ConsoleApp"
      location(ProjectsTempDir .. solutionName)
      targetname(solutionName)
      language "C++"

      files { "Source/**.c", "Source/**.cpp", "Source/**.h" }

      LinkLibraries(EssentialLibraries)
      if extraLibraries["Core"] ~= nil then
        LinkLibraries(extraLibraries["Core"])
      end
      if extraLibraries["Tool"] ~= nil then
        LinkLibraries(extraLibraries["Tool"])
      end

    configuration { platformName }
      for i, includedir in ipairs(extraIncludeDirs) do
        includedirs { MiddlewareDir .. includedir }
      end

      for i, postBuildCopy in ipairs(postBuildCopies) do
        postbuildcommands { "copy " .. MiddlewareCopyDir:gsub("/","\\") .. postBuildCopy }
      end

      for i, middlewareLinkDir in ipairs(extraMiddlewareLibDirs) do
        libdirs { MiddlewareDir .. middlewareLinkDir }
      end

      for i, middlewareLib in ipairs(extraMiddlewareLibraries) do
        links { middlewareLib }
      end

    -- LIB PROJECTS
    AddCoreLibraries(platformName, solutionName, EssentialLibraries)
    if extraLibraries["Core"] ~= nil then
      AddCoreLibraries(platformName, solutionName, extraLibraries["Core"])
    end
    if extraLibraries["Tool"] ~= nil then
      AddToolLibraries(platformName, solutionName, extraLibraries["Tool"])
    end
end