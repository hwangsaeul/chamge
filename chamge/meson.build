chamge_install_header_subdir = join_paths(chamge_api_name, 'chamge')

configure_file(output: 'config.h', configuration: cdata)

source_h = [
  'types.h',
  'node.h',
  'arbiter.h',
  'arbiter-backend.h',
  'edge.h',
  'edge-backend.h',
]

source_c = [
  'node.c',
  'arbiter.c',
  'arbiter-backend.c',
  'mock-arbiter-backend.c',
  'edge.c',
  'edge-backend.c',
  'mock-edge-backend.c',
  'amqp-edge-backend.c',
]

install_headers(source_h, subdir: chamge_install_header_subdir)

chamge_c_args = [
  '-DG_LOG_DOMAIN="CHAMGE"',
  '-DCHAMGE_COMPILATION',
]

chamge_enums = gnome.mkenums_simple(
  'enumtypes',
  header_prefix: '#include <chamge/types.h>',
  sources: source_h,
  decorator: 'CHAMGE_API_EXPORT',
  install_dir: chamge_install_header_subdir,
  install_header: true,
)

chamge_enums_h = chamge_enums[1]
chamge_enums_c = chamge_enums[0]

libchamge = library(
  'chamge-@0@'.format(apiversion),
  chamge_enums, source_c,
  version: libversion,
  soversion: soversion,
  include_directories: chamge_incs,
  dependencies: [ gobject_dep ],
  c_args: chamge_c_args,
  link_args: common_ldflags,
  install: true
)

libchamge_dep = declare_dependency(link_with: libchamge,
  include_directories: [ chamge_incs ],
  dependencies: [ gobject_dep, rabbitmq_dep ],
  sources: [ chamge_enums_h ],
)

subdir('dbus')
