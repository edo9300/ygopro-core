project('ocgcore', 'cpp',
	license : 'AGPL-3.0-or-later',
	default_options : ['cpp_std=c++17', 'warning_level=3', 'cpp_rtti=false'],
	version : '11.0'
)

ocg_args = []
if get_option('default_library') == 'shared'
	ocg_args += ['-DOCGCORE_EXPORT_FUNCTIONS']
	if meson.get_compiler('cpp').get_id() != 'msvc'
		add_global_arguments('-fvisibility=hidden', language: ['cpp'])
	endif
endif

subdir('lua')

linker_args = []
if meson.get_compiler('cpp').get_id() != 'msvc'
	linker_args += ['-Wl,--no-undefined']
endif

if host_machine.system() == 'darwin'
	ocg_args += ['-D_LIBCPP_NO_EXCEPTIONS']
endif

ocgcore_src = files([
	'card.cpp',
	'duel.cpp',
	'effect.cpp',
	'field.cpp',
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
	'processor_visit.cpp',
	'scriptlib.cpp',
])

library('ocgcore', ocgcore_src, cpp_args : ocg_args, link_args : linker_args, dependencies : lua_dep)
