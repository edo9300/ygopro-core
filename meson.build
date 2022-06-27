project('ocgcore', 'cpp',
	license : 'AGPL-3.0-or-later',
	default_options : ['cpp_std=c++14'],
	version : '9.1'
)

lua_dep = dependency('lua-5.4')

args = []
if get_option('default_library') == 'shared'
	args += ['-DOCGCORE_EXPORT_FUNCTIONS', '-fvisibility=hidden']
endif

ocgcore_src = files([
	'card.cpp',
	'duel.cpp',
	'effect.cpp',
	'field.cpp',
	'group.cpp',
	'interpreter.cpp',
	'libcard.cpp',
	'libdebug.cpp',
	'libduel.cpp',
	'libeffect.cpp',
	'libgroup.cpp',
	'ocgapi.cpp',
	'operations.cpp',
	'playerop.cpp',
	'processor.cpp',
	'scriptlib.cpp',
])

library('ocgcore', ocgcore_src, cpp_args : args, dependencies : lua_dep)
