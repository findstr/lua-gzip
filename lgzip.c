#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#define	ZLIB_CONST
#include "zlib.h"

#define BUFFSIZE (128)

void *
zmalloc(void *q, unsigned n, unsigned m)
{
    (void)q;
    return calloc(n, m);
}

static void
zfree(void *q, void *p)
{
    (void)q;
    free(p);
}

static inline void
setfuncswithbuffer(lua_State *L, luaL_Reg tbl[])
{
	int i = 0;
	while (tbl[i].name) {
		lua_newuserdata(L, BUFFSIZE);
		lua_pushcclosure(L, tbl[i].func, 1);
		lua_setfield(L, -2, tbl[i].name);
		i++;
	}
}

static inline void *
expandbuffer(lua_State *L, size_t sz)
{
	void *old = lua_touserdata(L, lua_upvalueindex(1));
	void *data = lua_newuserdata(L, sz * 2);
	memcpy(data, old, sz);
	lua_replace(L, lua_upvalueindex(1));
	return data;
}

static inline void *
funcbuffer(lua_State *L, size_t *sz)
{
	*sz = lua_rawlen(L, lua_upvalueindex(1));
	return lua_touserdata(L, lua_upvalueindex(1));
}

static int
ldeflategz(lua_State *L)
{
	int err;
	size_t outsz;
	size_t inputsz;
	unsigned char *output;
	const unsigned char *input;
	input = (unsigned char *)luaL_checklstring(L, 1, &inputsz);
	output = funcbuffer(L, &outsz);
	z_stream c_stream; /* compression stream */
	c_stream.zalloc = zmalloc;
	c_stream.zfree = zfree;
	c_stream.opaque = (voidpf)0;
	err = deflateInit2(&c_stream,
			Z_BEST_SPEED,
			Z_DEFLATED,
			MAX_WBITS + 16,
			8,
			Z_DEFAULT_STRATEGY);
	if (err != Z_OK)
		return luaL_error(L, "gzip deflateInit2 error:%d", err);
	c_stream.next_out = output;
	c_stream.avail_out = outsz;
	c_stream.next_in = input;
	c_stream.avail_in = inputsz;
	for (;;) {
		err = deflate(&c_stream, Z_NO_FLUSH);
		if (err != Z_OK) {
			return luaL_error(L,
				"gzip deflate error:%d", err);
		}
		if (c_stream.avail_out == 0) {
			output = expandbuffer(L, outsz);
			c_stream.avail_out = outsz;
			c_stream.next_out = output + outsz;
			outsz *= 2;
		} else {
			break;
		}
	}
	for (;;) {
		err = deflate(&c_stream, Z_FINISH);
		if (err == Z_STREAM_END)
			break;
		if (err == Z_OK) {
			output = expandbuffer(L, outsz);
			c_stream.avail_out = outsz;
			c_stream.next_out = output + outsz;
			outsz *= 2;
		} else {
			return luaL_error(L,
				"gzip deflate finish error:%d", err);
		}
	}
	err = deflateEnd(&c_stream);
	if (err != Z_OK)
		return luaL_error(L, "gzip deflate end error:%d", err);
	lua_pushlstring(L, (char *)output, c_stream.total_out);
	return 1;
}

static int
linflategz(lua_State *L)
{
	int ret;
	size_t outputsz;
	size_t inputsz;
	unsigned char *output;
	const unsigned char *input;
	z_stream d_stream; /* decompression stream */

	input = (unsigned char *)luaL_checklstring(L, 1, &inputsz);
	output = funcbuffer(L, &outputsz);


	d_stream.zalloc = zmalloc;
	d_stream.zfree = zfree;
	d_stream.opaque = (voidpf)0;
	d_stream.next_in  = input;
	d_stream.avail_in = inputsz;

	ret = inflateInit2(&d_stream, MAX_WBITS + 16);
	if (ret != Z_OK)
		return luaL_error(L, "gzip inflateInit2 error:%d", ret);
	d_stream.next_out = output;
	d_stream.avail_out = outputsz;
	for (;;) {
		ret = inflate(&d_stream, Z_NO_FLUSH);
		if (ret == Z_STREAM_END)
			break;
		if (ret != Z_OK) {
			return luaL_error(L,
				"gzip inflate error:%d", ret);
		}
		if (d_stream.avail_out == 0) {
			output = expandbuffer(L, outputsz);
			d_stream.avail_out = outputsz;
			d_stream.next_out = output + outputsz;
			outputsz *= 2;
		}
	}
	ret = inflateEnd(&d_stream);
	if (ret != Z_OK)
		return luaL_error(L, "gzip inflate end error:%d", ret);
	lua_pushlstring(L, (char *)output, d_stream.total_out);
	return 1;
}

int
luaopen_gzip(lua_State *L)
{
	luaL_Reg tbl[] = {
		{"deflate", ldeflategz},
		{"inflate", linflategz},
		{NULL, NULL},
	};
	luaL_checkversion(L);
	luaL_newlibtable(L, tbl);
	setfuncswithbuffer(L, tbl);
	return 1;
}

