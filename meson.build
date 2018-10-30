project('photon', ['c', 'cpp'], default_options : ['cpp_std=c++11', 'c_std=c99'])

cc = meson.get_compiler('c')

c_args = []
if cc.get_id() == 'clang' or cc.get_id() == 'gcc'
  c_args = ['-Werror=implicit-function-declaration', '-Wno-unused-lambda-capture']
endif

add_project_arguments(c_args, language : 'c')

platform_deps = declare_dependency()
if host_machine.system() == 'windows'
  platform_deps = declare_dependency(dependencies : meson.get_compiler('c').find_library('ws2_32'))
endif

qt5 = import('qt5')
qt5_dep = dependency('qt5', modules: ['Core', 'Gui', 'Widgets'])

decode = subproject('decode')
decode_gen = decode.get_variable('decode_gen')

project_path = join_paths(get_option('project_prefix'), get_option('project'))

modules_src = [
  project_path,
  'modules/photon/asv/Stubs.c',
  'modules/photon/asv/asv.decode',
  'modules/photon/asv/mod.toml',
  'modules/photon/blog/Blog.c',
  'modules/photon/blog/blog.decode',
  'modules/photon/blog/mod.toml',
  'modules/photon/clk/Clk.c',
  'modules/photon/clk/clk.decode',
  'modules/photon/clk/mod.toml',
  'modules/photon/core/Assert.h',
  'modules/photon/core/Config.h',
  'modules/photon/core/core.decode',
  'modules/photon/core/Crc.c',
  'modules/photon/core/Crc.h',
  'modules/photon/core/Endian.h',
  'modules/photon/core/Logging.c',
  'modules/photon/core/Logging.h',
  'modules/photon/core/Main.c',
  'modules/photon/core/mod.toml',
  'modules/photon/core/Reader.c',
  'modules/photon/core/RingBuf.c',
  'modules/photon/core/Try.h',
  'modules/photon/core/Util.h',
  'modules/photon/core/Writer.c',
  'modules/photon/exc/DataPacket.c',
  'modules/photon/exc/Device.c',
  'modules/photon/exc/Exc.c',
  'modules/photon/exc/exc.decode',
  'modules/photon/exc/mod.toml',
  'modules/photon/exc/ReceiptPacket.c',
  'modules/photon/exc/Utils.h',
  'modules/photon/fcu/fcu.decode',
  'modules/photon/fcu/mod.toml',
  'modules/photon/fcu/Fcu.c',
  'modules/photon/fl/Fl.c',
  'modules/photon/fl/fl.decode',
  'modules/photon/fl/mod.toml',
  'modules/photon/fwt/Fwt.c',
  'modules/photon/fwt/fwt.decode',
  'modules/photon/fwt/mod.toml',
  'modules/photon/dfu/Dfu.c',
  'modules/photon/dfu/dfu.decode',
  'modules/photon/dfu/mod.toml',
  'modules/photon/grp/Grp.c',
  'modules/photon/grp/grp.decode',
  'modules/photon/grp/mod.toml',
  'modules/photon/nav/mod.toml',
  'modules/photon/nav/Nav.c',
  'modules/photon/nav/nav.decode',
  'modules/photon/pvu/mod.toml',
  'modules/photon/pvu/Pvu.c',
  'modules/photon/pvu/pvu.decode',
  'modules/photon/test/mod.toml',
  'modules/photon/test/Test.c',
  'modules/photon/test/test.decode',
  'modules/photon/tm/mod.toml',
  'modules/photon/tm/StatusMessage.c',
  'modules/photon/tm/Tm.c',
  'modules/photon/tm/tm.decode',
  'modules/photon/usm/usm.decode',
  'modules/photon/usm/mod.toml',
  'modules/photon/usm/Stubs.c',
  'modules/photon/param/mod.toml',
  'modules/photon/param/param.decode',
  'modules/photon/param/Param.c',
  'modules/photon/fcu/mod.toml',
  'modules/photon/fcu/fcu.decode',
  'modules/photon/fcu/Fcu.c',
] + get_option('additional_depends')

groundcontrol_src = [
  'src/photon/groundcontrol/AllowUnsafeMessageType.h',
  'src/photon/groundcontrol/Atoms.h',
  'src/photon/groundcontrol/CmdState.cpp',
  'src/photon/groundcontrol/CmdState.h',
  'src/photon/groundcontrol/Crc.cpp',
  'src/photon/groundcontrol/Crc.h',
  'src/photon/groundcontrol/DfuState.cpp',
  'src/photon/groundcontrol/DfuState.h',
  'src/photon/groundcontrol/Exchange.cpp',
  'src/photon/groundcontrol/Exchange.h',
  'src/photon/groundcontrol/FwtState.cpp',
  'src/photon/groundcontrol/FwtState.h',
  'src/photon/groundcontrol/GcInterface.cpp',
  'src/photon/groundcontrol/GcInterface.h',
  'src/photon/groundcontrol/GcStructs.h',
  'src/photon/groundcontrol/GroundControl.cpp',
  'src/photon/groundcontrol/GroundControl.h',
  'src/photon/groundcontrol/MemIntervalSet.cpp',
  'src/photon/groundcontrol/MemIntervalSet.h',
  'src/photon/groundcontrol/ProjectUpdate.cpp',
  'src/photon/groundcontrol/ProjectUpdate.h',
  'src/photon/groundcontrol/SerialStream.cpp',
  'src/photon/groundcontrol/SerialStream.h',
  'src/photon/groundcontrol/StreamFromString.cpp',
  'src/photon/groundcontrol/StreamFromString.h',
  'src/photon/groundcontrol/TmParamUpdate.h',
  'src/photon/groundcontrol/TmState.cpp',
  'src/photon/groundcontrol/TmState.h',
  'src/photon/groundcontrol/UdpStream.cpp',
  'src/photon/groundcontrol/UdpStream.h',
]

model_src = [
  'src/photon/model/CmdModel.cpp',
  'src/photon/model/CmdModel.h',
  'src/photon/model/CmdNode.cpp',
  'src/photon/model/CmdNode.h',
  'src/photon/model/Decoder.cpp',
  'src/photon/model/Decoder.h',
  'src/photon/model/Encoder.cpp',
  'src/photon/model/Encoder.h',
  'src/photon/model/FieldsNode.cpp',
  'src/photon/model/FieldsNode.h',
  'src/photon/model/FindNode.cpp',
  'src/photon/model/FindNode.h',
  'src/photon/model/Node.cpp',
  'src/photon/model/Node.h',
  'src/photon/model/NodeView.cpp',
  'src/photon/model/NodeView.h',
  'src/photon/model/NodeViewStore.cpp',
  'src/photon/model/NodeViewStore.h',
  'src/photon/model/NodeViewUpdate.cpp',
  'src/photon/model/NodeViewUpdate.h',
  'src/photon/model/NodeViewUpdater.cpp',
  'src/photon/model/NodeViewUpdater.h',
  'src/photon/model/OnboardTime.cpp',
  'src/photon/model/OnboardTime.h',
  'src/photon/model/TmMsgDecoder.cpp',
  'src/photon/model/TmMsgDecoder.h',
  'src/photon/model/TmModel.cpp',
  'src/photon/model/TmModel.h',
  'src/photon/model/Value.cpp',
  'src/photon/model/Value.h',
  'src/photon/model/ValueInfoCache.cpp',
  'src/photon/model/ValueInfoCache.h',
  'src/photon/model/ValueKind.h',
  'src/photon/model/ValueNode.cpp',
  'src/photon/model/ValueNode.h',
]

ui_headers = [
  'src/photon/ui/FirmwareWidget.h',
  'src/photon/ui/FirmwareStatusWidget.h',
  'src/photon/ui/QModelBase.h',
  'src/photon/ui/QCmdModel.h',
  'src/photon/ui/QNodeModel.h',
  'src/photon/ui/QNodeViewModel.h',
]

inc = include_directories('src')

ui_moc = qt5.preprocess(
  moc_headers : ui_headers,
  include_directories: inc,
)

ui_src = [
  'src/photon/ui/FirmwareWidget.cpp',
  'src/photon/ui/FirmwareStatusWidget.cpp',
  'src/photon/ui/QModelBase.cpp',
  'src/photon/ui/QCmdModel.cpp',
  'src/photon/ui/QNodeModel.cpp',
  'src/photon/ui/QNodeViewModel.cpp',
] + ui_moc

bmcl = subproject('bmcl', default_options: ['build_tests=false'])
caf = subproject('caf')
asio = subproject('asio', default_options: ['shared_lib=false'])
tclap = subproject('tclap')

thread_dep = dependency('threads')
deps = [
  platform_deps,
  thread_dep,
  qt5_dep,
  decode.get_variable('libdecode_dep'),
  bmcl.get_variable('bmcl_dep'),
  caf.get_variable('libcaf_core_dep'),
  asio.get_variable('asio_dep'),
  tclap.get_variable('tclap_dep'),
]

abs_paths_flag = ''

if get_option('abs_paths')
  abs_paths_flag = '-a'
endif

gen_src = custom_target('photon-generated-src',
  input : project_path,
  output : ['Photon.c', 'Photon.h', 'Photon.cpp', 'Photon.hpp'],
  depend_files : modules_src,
  command : [decode_gen, '-p', '@INPUT@', '-o', meson.current_build_dir(), '-c', '4', abs_paths_flag],
)

photon_c = gen_src[0]
photon_h = gen_src[1]
photon_cpp = gen_src[2]
photon_hpp = gen_src[3]

gen_src_headers = [photon_hpp, photon_h]

gen_src_dep = declare_dependency(
  sources: gen_src_headers,
)

gen_onboard_src = [photon_c, photon_h]
gen_gc_src = [photon_cpp, photon_hpp]

gc_inc = include_directories('.', 'src')

photon_lib = static_library('photon',
  sources : ui_src + groundcontrol_src + model_src + [gen_gc_src[0]],
  include_directories: gc_inc,
  dependencies: [deps, gen_src_dep],
  cpp_args: '-DBUILDING_PHOTON',
)

photon_dep = declare_dependency(
  link_with : photon_lib,
  include_directories: gc_inc,
  dependencies: [deps, gen_src_dep],
)

test_moc = qt5.preprocess(
#  moc_headers : 'tests/UiTest.h',
  include_directories: gc_inc,
)

uitest_lib = static_library('photon-uitest',
  sources : ['tests/UiTest.cpp', test_moc],
  include_directories: gc_inc,
  link_with: photon_lib,
  dependencies: [deps, gen_src_dep],
)

client_tests = [
  ['dfu', 'Dfu.cpp'],
  ['client', 'Client.cpp'],
  ['client-serial', 'ComTest.cpp'],
  ['client-udp', 'UdpTest.cpp'],
]

foreach test : client_tests
  executable('photon-' + test[0],
    include_directories : gc_inc,
    sources : 'tests/' + test[1],
    dependencies: [deps, gen_src_dep],
    link_with : uitest_lib,
  )
endforeach

tools = [
  ['blog-info', 'BlogInfo.cpp'],
  ['proxy', 'Proxy.cpp'],
]

foreach tool : tools
  executable('photon-' + tool[0],
    include_directories : gc_inc,
    sources : 'tools/' + tool[1],
    dependencies: [deps, gen_src_dep],
    link_with : photon_lib,
  )
endforeach

onboard_flags = []
if get_option('use_stubs')
  onboard_flags += '-DPHOTON_STUB'
endif

if get_option('abs_paths')
  onboard_inc = include_directories('.', 'modules')
else
  onboard_inc = include_directories('.')
endif

photon_targets = get_option('targets')

model_tests = [
  ['model-inproc', 'InprocTest.cpp'],
  ['model-udpserver', 'Model.cpp'],
]

log_level = get_option('log_level').to_int()

if log_level > 0
  onboard_flags += '-DPHOTON_LOG_LEVEL=' + log_level.to_string()
endif

foreach name : photon_targets
  libname = 'photon-target-' + name
  flag = '-DPHOTON_DEVICE_' + name.to_upper()

  lib = static_library(libname,
    sources : gen_onboard_src,
    include_directories : onboard_inc,
    c_args : [onboard_flags, flag],
  )
  set_variable('photon_target_' + name + '_lib', lib)

  dep = declare_dependency(
    link_with : lib,
    include_directories : onboard_inc,
    compile_args : onboard_flags + [flag],
  )
  set_variable('photon_target_' + name + '_dep', dep)

  if get_option('target_tests')
    foreach test : model_tests
      executable('photon-' + test[0] + '-' + name,
        include_directories : [gc_inc, onboard_inc],
        sources : 'tests/' + test[1],
        dependencies: [deps, gen_src_dep],
        link_with : [uitest_lib, lib],
        cpp_args : onboard_flags + [flag],
      )
    endforeach
  endif
endforeach

#TODO: tests
