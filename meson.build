project('PretextView', ['c', 'cpp'],
    license: 'MIT',
    meson_version: '>=0.57.1',
    version: '0.2.5'
)

flags = ['-Ofast']
link_ags = ['-Ofast']
if get_option('buildtype') == 'debug'
    flags = ['-O0', '-g', '-DDEBUG', '-DInternal']
    link_ags = ['-O0']
endif
flags += ['-DPV=' + meson.project_version()]


deps = [dependency('threads')]
# flags += ['-mmacosx-version-min=10.10']
# link_ags += ['-mmacosx-version-min=10.10']
flags += ['-mmacosx-version-min=14.3']
link_ags += ['-mmacosx-version-min=14.3']
deps += [dependency('appleframeworks', modules : 'Cocoa')]
deps += [dependency('appleframeworks', modules : 'OpenGL')]
deps += [dependency('appleframeworks', modules : 'IOKit')]
deps += [dependency('appleframeworks', modules : 'CoreVideo')]

# lib of deflate
libdeflate =  static_library('deflate', 'libdeflate/lib/deflate_compress.c', 'libdeflate/lib/deflate_decompress.c', 'libdeflate/lib/utils.c', c_args : flags, link_with : 
                [   
                    static_library('deflate_x86', 'libdeflate/lib/x86/cpu_features.c', c_args : flags, include_directories : 'libdeflate'),
                    static_library('deflate_arm', 'libdeflate/lib/arm/cpu_features.c', c_args : flags, include_directories : 'libdeflate')], include_directories : 'libdeflate')

# libglfw
cmake = import('cmake')
glfw_ops = cmake.subproject_options()
glfw_ops.add_cmake_defines({'GLFW_BUILD_EXAMPLES' : false, 'GLFW_BUILD_DOCS' : false, 'GLFW_INSTALL' : false, 'GLFW_BUILD_TESTS' : false})
glfw_ops.append_compile_args('c', flags)
glfw_ops.append_compile_args('cpp', flags)
glfw_ops.append_link_args(['c', 'cpp'], link_ags)
deps += [cmake.subproject('glfw', options : glfw_ops).dependency('glfw')]

executable('PretextView', ['PretextView.cpp', 'glad.c'], 
    dependencies : deps, 
    cpp_args : flags + ['--std=c++17'], 
    c_args : flags + ['--std=c17'], 
    link_args : link_ags, 
    link_with : libdeflate, 
    include_directories : ['include', 'libdeflate'], 
    install : true)

# gernate the macOS application, including the verion number, name, icon, ...
if host_machine.system() == 'darwin' 
    custom_target('plist', command : [find_program('make_macos_app_plist.sh'), meson.project_version()], build_by_default : true, build_always_stale : true, output : ['Info.plist'], install : true, install_dir : 'Contents')
    install_data('icon.icns', install_dir : 'Contents/Resources')
endif
