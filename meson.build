project('ftdi_bitbang','c')

cc = meson.get_compiler('c')
build_target = ['src/main.c']
include_dir = 'inc'
ftd2_dep = cc.find_library('ftd2xx', dirs:['/usr/local/lib'],required : false)
rt_dep = cc.find_library('rt',required : false)


executable('ftdi_bitbang',build_target,include_directories : include_dir,dependencies : [ftd2_dep,rt_dep])