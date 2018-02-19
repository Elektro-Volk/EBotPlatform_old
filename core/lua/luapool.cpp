#include "luawork.h"
#include "luapool.h"
#include "luai.h"
#include "lJson.h"
#include "lAPI.h"
#include "../cvar.h"
#include "../console.h"

using json = nlohmann::json;

char luapool::nPools;
std::vector<luapool::pool*> luapool::pools;

void luapool::start()
{
  for(int i = 0; i<3; i++)
    pools.push_back(new pool());
}

void luapool::close()
{
  con::log("Closing pools...");
  for(int i=pools.size(); i<pools.size(); i++) {
    pools[i]->sRemove();
    while(true) if(!pools[i]->active) break;
    delete pools[i];
  }
  pools.clear();
  con::log("Pools closed.");
}

bool pushValue(lua_State *L, json &value);
bool checkMessage(lua_State *state, json msg)
{
  lua_getglobal(state, "CheckMessage");
  pushValue(state, msg);
  luawork::safeCall(state, 1, 1);
  return !lua_isnil(state, -1);
}

void luapool::add(nlohmann::json msg)
{
    if(checkMessage(luawork::state, msg)) {
      // find free pool and add msg then
      while(true){
        pool *p = nullptr;
        for(int i=0; i<pools.size(); i++) {
          if(pools[i]->isFree()) {
            p = pools[i];
            break;
          }
        }

        if(p) {
          p->add();
          break;
        }
      }
    }
    lua_settop(luawork::state, 0);
}


// Class
luapool::pool::pool()
{
  // params
  this->id = nPools++;
  this->free = true;
	this->toremove = false;
  this->have = false;
  this->active = true;
  // Lua thread
	//lua_lock(G_L);
	this->L = lua_newthread(luawork::state);
	lua_setfield(luawork::state, LUA_REGISTRYINDEX, &this->id);
	//lua_unlock(G_L);
  // C thread
	std::thread t(&luapool::pool::loop, this);
	t.detach();
}

bool luapool::pool::isFree()
{
  return this->free;
}

void luapool::pool::sRemove()
{
  toremove = true;
}
void stackDump (lua_State *L);
void luapool::pool::add()
{
  this->free = false;
  lua_settop(L, 0);
  lua_xmove (luawork::state, L, 2);
  //stackDump (L);
  this->have = true;
}

void luapool::pool::loop()
{
  while (!toremove) {
		if (this->have && !this->free) {
      lua_getglobal(L, "NewMessage");
      lua_pushvalue (L, 2);
      luawork::safeCall(L, 1);
    }

		this->free = true;
    this->have = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

  active = false;
}
