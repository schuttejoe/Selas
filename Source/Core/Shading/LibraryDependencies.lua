
local platform = ...

loadfile(RootDirectory .. "ProjectGen\\Middlewares\\embree.lua")(platform)
loadfile(RootDirectory .. "ProjectGen\\Middlewares\\ptex.lua")(platform)