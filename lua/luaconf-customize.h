#if defined(LUA_EPRO_APICHECK)
void ocgcore_lua_api_check(void* L, const char* error_message);
#define luai_apicheck(l,e)	do { if(!e) ocgcore_lua_api_check(l,"Lua api check failed: " #e); } while(0)
#endif
#if defined(__ANDROID__)
#define lua_getlocaledecpoint() '.'
#endif
