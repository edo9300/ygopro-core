# we need to build lua as a c++ project and also have its sources include the custom configuration header
cat = find_program('cat', required: false)
if cat.found()
	gen = generator(cat,
                output  : '@BASENAME@.cpp',
                arguments : [meson.current_source_dir() / 'luaconf-customize.h', '@INPUT@'],
				capture : true)
else
	cmd = find_program('cmd')
	gen = generator(cmd,
                output  : '@BASENAME@.cpp',
                arguments : ['/c', 'type', meson.current_source_dir() / 'luaconf-customize.h', '@INPUT@'],
				capture : true)
endif

lua_args = []
if get_option('lua_apicheck') == true
	lua_args += ['-DLUA_EPRO_APICHECK']
endif

lua_inc = include_directories('src/')
lua_lib = static_library(
	'lua',
	include_directories: [
		include_directories(
			'src/',
		)
	],
	sources: [
		gen.process([
			'src/lapi.c',
			'src/lauxlib.c',
			'src/lbaselib.c',
			'src/lcode.c',
			'src/lctype.c',
			'src/ldebug.c',
			'src/ldo.c',
			'src/ldump.c',
			'src/lfunc.c',
			'src/lgc.c',
			'src/liolib.c',
			'src/llex.c',
			'src/lmathlib.c',
			'src/lmem.c',
			'src/lobject.c',
			'src/lopcodes.c',
			'src/lparser.c',
			'src/lstate.c',
			'src/lstring.c',
			'src/lstrlib.c',
			'src/ltable.c',
			'src/ltablib.c',
			'src/ltm.c',
			'src/lundump.c',
			'src/lvm.c',
			'src/lzio.c',
		]),
	],
	cpp_args : lua_args,
	override_options: ['warning_level=0'],
)

lua_dep = declare_dependency(link_with : lua_lib, include_directories : lua_inc)
