#include "module.hpp"
#include <goluau.h>
#include <format>
#include <chrono>
#include "Type.hpp"
#include <string>
using namespace std::string_literals;
using TimeType = Type<Time>;
static const std::string tname = module_name + "."s + "Time";
template<> const char* TimeType::type_name(){return tname.c_str();}

Time* to_time(lua_State* L, int idx) {
    return &TimeType::check_udata(L, idx);
}

static const TimeType::Registry namecall = {
    {"format", [](lua_State* L, Time& tp) -> int {
        const std::string fmt = "{:"s + luaL_checkstring(L, 2) + "}"s;
        lua_pushstring(L, std::vformat(fmt, std::make_format_args(tp)).c_str());
        return 1;
    }},
    {"type", [](lua_State* L, Time& d) -> int {
        lua_pushstring(L, tname.c_str());
        return 1;
    }},
};

static const TimeType::Registry index = {
    {"year", [](lua_State* L, Time& tp) -> int {
        ch::year_month_day ymd{ch::floor<ch::days>(tp)};
        lua_pushinteger(L, static_cast<int>(ymd.year()));
        return 1;
    }},
    {"month", [](lua_State* L, Time& tp) -> int {
        ch::year_month_day ymd{ch::floor<ch::days>(tp)};
        lua_pushinteger(L, static_cast<unsigned int>(ymd.month()));
        return 1;
    }},
    {"day", [](lua_State* L, Time& tp) -> int {
        ch::year_month_day ymd{ch::floor<ch::days>(tp)};
        lua_pushinteger(L, static_cast<unsigned int>(ymd.day()));
        return 1;
    }},
    {"hour", [](lua_State* L, Time& tp) -> int {
        const auto midnight = tp - ch::floor<ch::days>(tp);
        const auto hours = ch::duration_cast<ch::hours>(midnight);
        lua_pushinteger(L, hours.count());
        return 1;
    }},
    {"minute", [](lua_State* L, Time& tp) -> int {
        const auto midnight = tp - ch::floor<ch::days>(tp);
        const auto hours = ch::duration_cast<ch::hours>(midnight);
        const auto minutes = ch::duration_cast<ch::minutes>(midnight - hours);
        lua_pushinteger(L, minutes.count());
        return 1;
    }},
    {"second", [](lua_State* L, Time& tp) -> int {
        const auto midnight = tp - ch::floor<ch::days>(tp);
        const auto hours = ch::duration_cast<ch::hours>(midnight);
        const auto minutes = ch::duration_cast<ch::minutes>(midnight - hours);
        const auto seconds = ch::duration_cast<ch::seconds>(midnight - hours - minutes);
        lua_pushinteger(L, seconds.count());
        return 1;
    }},
};

static int tostring(lua_State* L) {
    Time& tp = TimeType::check_udata(L, 1);
    lua_pushstring(L, std::format("{}", tp).c_str());
    return 1;
}

Time& new_time(lua_State* L, const Time& v) {
    if (not TimeType::initialized(L)) {
        const luaL_Reg meta[] = {
            {"__tostring", tostring},
            {nullptr, nullptr}
        };
        TimeType::init(L, {
            .index = index,
            .namecall = namecall,
            .meta = meta,
        });
    }
    return TimeType::new_udata(L, v);
}