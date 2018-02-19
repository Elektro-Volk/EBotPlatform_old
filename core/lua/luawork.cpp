#include "luawork.h"
#include "luai.h"
#include "lJson.h"
#include "lAPI.h"
#include "../cvar.h"
#include "../console.h"
#include "../cmd.h"
#include "luaModules.h"
#include "luapool.h"

using json = nlohmann::json;

lua_State *luawork::state;
string luawork::script_path = "main.lua";
bool luawork::isWorking = false;

void luawork::init()
{
  cvars::add("lua_script", &script_path);
  cmd::add("relua", luawork::c_relua, "Reload lua scripts");
  cmd::add("mlist", luaModules::c_mlist, "Lua modules list");
  cmd::add("madd", luaModules::c_madd, "Add module");
  cmd::add("mrem", luaModules::c_mrem, "Remove module");
  luaModules::load();
}

void luawork::start()
{
  try{
    state = luaL_newstate();
    luaL_openlibs(state);
    lapi::openApi(state);
    con::log("Loading modules...");
    luaModules::loadModules(state);
    if(luaL_loadfile(state, (bot_path + "/scripts/" + script_path).c_str()))
      throw new string("lua[load] -> " + string(lua_tostring(state, -1)));
    if(lua_pcall(state, 0, LUA_MULTRET, 0))
      throw new string("lua[start] -> " + string(lua_tostring(state, -1)));

    luapool::start();
    luawork::isWorking = true;
    con::log("Lua has been loaded.");
  }
  catch(string *str) {
    luawork::isWorking = false;
    con::error(*str);
    con::error("Fix all the Lua errors and reload Lua (relua)");
    delete str;
  }
}

void luawork::push(json msg)
{
  if(!luawork::isWorking) return;
  luapool::add(msg);
}

bool luawork::safeCall(lua_State* L, int argnum, int retnum)
{
  if(lua_pcall(L, argnum, retnum, 0)) {
    con::error("lua -> " + string(lua_tostring(L, -1)));
    return false;
  }
  return true;
}

void luawork::close()
{
  luawork::isWorking = false;
  luapool::close();
  lua_close(state);
}

string luawork::c_relua(vector<string> _args)
{
  luawork::close();
  luawork::start();
  return "Lua has been reloaded.";
}
