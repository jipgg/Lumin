#include "module.hpp"
#include <lumin.h>
#include <format>
#include <chrono>
#include "generic_userdata_template.hpp"
#include <string>
using namespace std::string_literals;
using gut = generic_userdata_template<high_precision_time>;
static const std::string tname = module_name + "."s + "high_precision_time";
template<> const char* gut::type_name(){return tname.c_str();}

high_precision_time& to_high_precision_time(lua_State* L, int idx) {
    return gut::check_udata(L, idx);
}

static const gut::registry namecall = {
    {"format", [](lua_State* L, high_precision_time& tp) -> int {
        const std::string fmt = "{:"s + luaL_checkstring(L, 2) + "}"s;
        lua_pushstring(L, std::vformat(fmt, std::make_format_args(tp)).c_str());
        return 1;
    }},
    {"type", [](lua_State* L, high_precision_time& d) -> int {
        lua_pushstring(L, tname.c_str());
        return 1;
    }},
};

static const gut::registry index = {
    {"hour", [](lua_State* L, high_precision_time& tp) -> int {
        const auto midnight = tp - ch::floor<ch::days>(tp);
        const auto hours = ch::duration_cast<ch::hours>(midnight);
        lua_pushinteger(L, hours.count());
        return 1;
    }},
    {"minute", [](lua_State* L, high_precision_time& tp) -> int {
        const auto midnight = tp - ch::floor<ch::days>(tp);
        const auto hours = ch::duration_cast<ch::hours>(midnight);
        const auto minutes = ch::duration_cast<ch::minutes>(midnight - hours);
        lua_pushinteger(L, minutes.count());
        return 1;
    }},
    {"second", [](lua_State* L, high_precision_time& tp) -> int {
        const auto midnight = tp - ch::floor<ch::days>(tp);
        const auto hours = ch::duration_cast<ch::hours>(midnight);
        const auto minutes = ch::duration_cast<ch::minutes>(midnight - hours);
        const auto seconds = ch::duration_cast<ch::seconds>(midnight - hours - minutes);
        lua_pushinteger(L, seconds.count());
        return 1;
    }},
    {"millisecond", [](lua_State* L, high_precision_time& tp) -> int {
        const auto midnight = tp - ch::floor<ch::days>(tp);
        const auto hours = ch::duration_cast<ch::hours>(midnight);
        const auto minutes = ch::duration_cast<ch::minutes>(midnight - hours);
        const auto seconds = ch::duration_cast<ch::seconds>(midnight - hours - minutes);
        const auto milliseconds = ch::duration_cast<ch::milliseconds>(midnight - hours - minutes - seconds);
        lua_pushinteger(L, milliseconds.count());
        return 1;
    }},
    {"microsecond", [](lua_State* L, high_precision_time& tp) -> int {
        const auto midnight = tp - ch::floor<ch::days>(tp);
        const auto hours = ch::duration_cast<ch::hours>(midnight);
        const auto minutes = ch::duration_cast<ch::minutes>(midnight - hours);
        const auto seconds = ch::duration_cast<ch::seconds>(midnight - hours - minutes);
        const auto milliseconds = ch::duration_cast<ch::milliseconds>(
            midnight - hours - minutes - seconds);
        const auto microseconds = ch::duration_cast<ch::microseconds>(
            midnight - hours - minutes - seconds - milliseconds);
        lua_pushinteger(L, microseconds.count());
        return 1;
    }},
    {"nanosecond", [](lua_State* L, high_precision_time& tp) -> int {
        const auto midnight = tp - ch::floor<ch::days>(tp);
        const auto hours = ch::duration_cast<ch::hours>(midnight);
        const auto minutes = ch::duration_cast<ch::minutes>(midnight - hours);
        const auto seconds = ch::duration_cast<ch::seconds>(midnight - hours - minutes);
        const auto milliseconds = ch::duration_cast<ch::milliseconds>(
            midnight - hours - minutes - seconds);
        const auto microseconds = ch::duration_cast<ch::microseconds>(
            midnight - hours - minutes - seconds - milliseconds);
        const auto nanoseconds = ch::duration_cast<ch::nanoseconds>(
            midnight - hours - minutes - seconds - milliseconds);
        lua_pushinteger(L, nanoseconds.count());
        return 1;
    }},
};

static int tostring(lua_State* L) {
    high_precision_time& tp = gut::check_udata(L, 1);
    lua_pushstring(L, std::format("{}", tp).c_str());
    return 1;
}

high_precision_time& new_high_precision_time(lua_State* L, const high_precision_time& v) {
    if (not gut::initialized(L)) {
        const luaL_Reg meta[] = {
            {"__tostring", tostring},
            {nullptr, nullptr}
        };
        gut::init(L, {
            .index = index,
            .namecall = namecall,
            .meta = meta,
        });
    }
    return gut::new_udata(L, v);
}