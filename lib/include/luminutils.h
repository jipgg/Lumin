#include ".goluau_api.h"
#include <lua.h>
#include <stdbool.h>

LUMIN_UTILS_API lua_State* luminU_loadscript(lua_State* L, const char* script_path, size_t len);
LUMIN_UTILS_API const char* luminU_spawnscript(lua_State* L, const char* script_path, size_t len);