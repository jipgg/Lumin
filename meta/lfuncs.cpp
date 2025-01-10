#include "lumin.h"
#include "luminfuncs.h"
#include <lualib.h>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <lualib.h>
#include <luacode.h>
#include <Luau/Common.h>
#include <filesystem>
#include "luacode.h"
#include "Require.h"
#include <cassert>
namespace fs = std::filesystem;
#ifdef _WIN32
#include <Windows.h>
static std::unordered_map<fs::path, HMODULE> dllmodules{};
#endif
struct global_options {
    int optimization_level = 2;
    int debug_level = 1;
} global_opts;
static bool codegen = true;
lua_CompileOptions* compile_options() {
    static lua_CompileOptions result = {};
    result.optimizationLevel = global_opts.optimization_level;
    result.debugLevel = global_opts.debug_level;
    result.typeInfoLevel = 1;
    return &result;
}
int luminF_loadstring(lua_State* L) {
    size_t l = 0;
    const char* s = luaL_checklstring(L, 1, &l);
    const char* chunkname = luaL_optstring(L, 2, s);
    lua_setsafeenv(L, LUA_ENVIRONINDEX, false);
    size_t outsize;
    char* bc = luau_compile(s, l, compile_options(), &outsize);
    std::string bytecode(s, outsize);
    std::free(bc);
    if (luau_load(L, chunkname, bytecode.data(), bytecode.size(), 0) == 0)
        return 1;
    lua_pushnil(L);
    lua_insert(L, -2); // put before error message
    return 2;          // return nil plus error message
}
static int finishrequire(lua_State* L)
{
    if (lua_isstring(L, -1))
        lua_error(L);

    return 1;
}

struct RuntimeRequireContext : public RequireResolver::RequireContext
{
    // In the context of the REPL, source is the calling context's chunkname.
    //
    // These chunknames have certain prefixes that indicate context. These
    // are used when displaying debug information (see luaO_chunkid).
    //
    // Generally, the '@' prefix is used for filepaths, and the '=' prefix is
    // used for custom chunknames, such as =stdin.
    explicit RuntimeRequireContext(std::string source)
        : source(std::move(source))
    {
    }

    std::string getPath() override
    {
        return source.substr(1);
    }

    bool isRequireAllowed() override
    {
        return isStdin() || (!source.empty() && source[0] == '@') || (!source.empty() && source[0] == '=');
    }

    bool isStdin() override
    {
        return source == "=stdin";
    }

    std::string createNewIdentifer(const std::string& path) override
    {
        return "@" + path;
    }

private:
    std::string source;
};

struct RuntimeCacheManager : public RequireResolver::CacheManager
{
    explicit RuntimeCacheManager(lua_State* L)
        : L(L)
    {
    }

    bool isCached(const std::string& path) override
    {
        luaL_findtable(L, LUA_REGISTRYINDEX, "_MODULES", 1);
        lua_getfield(L, -1, path.c_str());
        bool cached = !lua_isnil(L, -1);
        lua_pop(L, 2);

        if (cached)
            cacheKey = path;

        return cached;
    }

    std::string cacheKey;

private:
    lua_State* L;
};

struct RuntimeErrorHandler : RequireResolver::ErrorHandler
{
    explicit RuntimeErrorHandler(lua_State* L)
        : L(L)
    {
    }

    void reportError(const std::string message) override
    {
        luaL_errorL(L, "%s", message.c_str());
    }

private:
    lua_State* L;
};

int luminF_require(lua_State* L)
{
    std::string name = luaL_checkstring(L, 1);

    RequireResolver::ResolvedRequire resolvedRequire;
    {
        lua_Debug ar;
        lua_getinfo(L, 1, "s", &ar);

        RuntimeRequireContext requireContext{ar.source};
        RuntimeCacheManager cacheManager{L};
        RuntimeErrorHandler errorHandler{L};

        RequireResolver resolver(std::move(name), requireContext, cacheManager, errorHandler);

        resolvedRequire = resolver.resolveRequire(
            [L, &cacheKey = cacheManager.cacheKey](const RequireResolver::ModuleStatus status)
            {
                lua_getfield(L, LUA_REGISTRYINDEX, "_MODULES");
                if (status == RequireResolver::ModuleStatus::Cached)
                    lua_getfield(L, -1, cacheKey.c_str());
            }
        );
    }

    if (resolvedRequire.status == RequireResolver::ModuleStatus::Cached)
        return finishrequire(L);

    // module needs to run in a new thread, isolated from the rest
    // note: we create ML on main thread so that it doesn't inherit environment of L
    lua_State* GL = lua_mainthread(L);
    lua_State* ML = lua_newthread(GL);
    lua_xmove(GL, L, 1);

    // new thread needs to have the globals sandboxed
    luaL_sandboxthread(ML);
    // now we can compile & run module on the new thread
    size_t outsize;
    char* bytecode_data = luau_compile(resolvedRequire.sourceCode.data(), resolvedRequire.sourceCode.length(), compile_options(), &outsize);
    std::string bytecode{bytecode_data, outsize};
    std::free(bytecode_data);
    if (luau_load(ML, resolvedRequire.identifier.c_str(), bytecode.data(), bytecode.size(), 0) == 0)
    {
        int status = lua_resume(ML, L, 0);

        if (status == 0)
        {
            if (lua_gettop(ML) == 0)
                lua_pushstring(ML, "module must return a value");
            else if (!lua_istable(ML, -1) && !lua_isfunction(ML, -1))
                lua_pushstring(ML, "module must return a table or function");
        }
        else if (status == LUA_YIELD)
        {
            lua_pushstring(ML, "module can not yield");
        }
        else if (!lua_isstring(ML, -1))
        {
            lua_pushstring(ML, "unknown error while running module");
        }
    }

    // there's now a return value on top of ML; L stack: _MODULES ML
    lua_xmove(ML, L, 1);
    lua_pushvalue(L, -1);
    lua_setfield(L, -4, resolvedRequire.absolutePath.c_str());
    // L stack: _MODULES ML result
    return finishrequire(L);
}
int luminF_collectgarbage(lua_State* L) {
    const char* option = luaL_optstring(L, 1, "collect");
    if (strcmp(option, "collect") == 0) {
        lua_gc(L, LUA_GCCOLLECT, 0);
        return 0;
    }
    if (strcmp(option, "count") == 0) {
        int c = lua_gc(L, LUA_GCCOUNT, 0);
        lua_pushnumber(L, c);
        return 1;
    }
    luaL_error(L, "collectgarbage must be called with 'count' or 'collect'");
}

int luminF_dllimport(lua_State* L) {
#ifdef _WIN32
    fs::path dllpath = luaL_checkstring(L, 1);
    if (not dllpath.has_extension()) dllpath += ".dll";
    const char* symbol = luaL_checkstring(L, 2);
    if (auto it = dllmodules.find(dllpath); it == dllmodules.end()) {
        const std::string dllpath_str = dllpath.string();
        HMODULE hmodule = LoadLibrary(dllpath_str.c_str());
        if (hmodule == nullptr) {
            luaL_errorL(L, "Could not find DLL with name '%s'.", dllpath_str.c_str());
            return 0;
        }
        dllmodules.emplace(dllpath, hmodule);
    }
    HMODULE hmodule = dllmodules[dllpath];
    FARPROC proc = GetProcAddress(hmodule, symbol);
    if (proc == NULL) {
        luaL_errorL(L, "Could not find exported function symbol '%s' in DLL '%s'.", symbol, dllpath.c_str());
        FreeLibrary(hmodule);
        return 0;
    }
    lua_pushcfunction(L, (lua_CFunction)proc, symbol);
    return 1;
#else
    luaL_errorL(L, "dllimport is currently not supported for this platform");
#endif
}