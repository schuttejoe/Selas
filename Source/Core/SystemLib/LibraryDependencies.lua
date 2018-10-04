
local platform = ...

loadfile(RootDirectory .. "ProjectGen\\Middlewares\\winpixruntime.lua")(platform)
loadfile(RootDirectory .. "ProjectGen\\Middlewares\\tbb.lua")(platform)