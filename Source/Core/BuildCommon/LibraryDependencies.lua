
local platform = ...


loadfile(RootDirectory .. "ProjectGen\\Middlewares\\assimp.lua")(platform)
loadfile(RootDirectory .. "ProjectGen\\Middlewares\\rapidjson.lua")(platform)