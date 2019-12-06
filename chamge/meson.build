pkg = import('pkgconfig')

chamge_install_header_subdir = join_paths(chamge_api_name, 'chamge')

configure_file(output: 'config.h', configuration: cdata)

schema_xmls = [
  'org.hwangsaeul.Chamge1.Arbiter.AMQP.gschema.xml',
  'org.hwangsaeul.Chamge1.Edge.AMQP.gschema.xml',
  'org.hwangsaeul.Chamge1.Hub.AMQP.gschema.xml',
]

schema = gnome.compile_schemas (
  depend_files: files(schema_xmls),
)

if meson.version().version_compare('>= 0.45.1')

  gio_schemasdir = dependency('gio-2.0').get_pkgconfig_variable(
    'schemasdir',
    define_variable: [ 'datadir', join_paths(get_option('prefix'), get_option('datadir')) ],
    default: join_paths(get_option('prefix'), get_option('datadir'), 'glib-2.0/schemas'),
  )

else

  # only for ubuntu 16.04
  gio_schemasdir = join_paths(get_option('prefix'), get_option('datadir'), 'glib-2.0/schemas')

endif

foreach xml: schema_xmls
  install_data(
    xml,
    install_dir: gio_schemasdir,
  )
endforeach

source_h = [
  'common.h',
  'chamge.h',
  'types.h',
  'node.h',
  'arbiter.h',
  'arbiter-backend.h',
  'edge.h',
  'edge-backend.h',
  'hub.h',
  'hub-backend.h',
]

source_c = [
  'common.c',
  'types.c',
  'node.c',
  'arbiter.c',
  'arbiter-backend.c',
  'mock-arbiter-backend.c',
  'amqp-arbiter-backend.c',
  'edge.c',
  'edge-backend.c',
  'mock-edge-backend.c',
  'amqp-edge-backend.c',
  'hub.c',
  'hub-backend.c',
  'mock-hub-backend.c',
  'amqp-hub-backend.c',
  'amqp-source.c',
  '../hwangsaeul/application.c',
]

install_headers(source_h, subdir: chamge_install_header_subdir)

chamge_c_args = [
  '-DG_LOG_DOMAIN="CHAMGE"',
  '-DCHAMGE_COMPILATION',
]

chamge_enums = gnome.mkenums(
  'enumtypes.h',
  h_template: 'enumtypes.h.template',
  c_template: 'enumtypes.c.template',
  sources: source_h,
  install_dir: join_paths(get_option('includedir'), chamge_install_header_subdir),
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
  dependencies: [ gobject_dep, gio_dep, rabbitmq_dep, json_glib_dep ],
  c_args: chamge_c_args,
  link_args: common_ldflags,
  install: true
)

libchamge_dep = declare_dependency(link_with: libchamge,
  include_directories: [ chamge_incs ],
  dependencies: [ gobject_dep, gio_dep, rabbitmq_dep ],
  sources: [ chamge_enums_h, schema ],
)

subdir('dbus')

if meson.version().version_compare('>= 0.48.2')

  pkg.generate(libchamge,
    libraries : [ libchamge_dbus_dep, rabbitmq_dep, json_glib_dep ],
    name : meson.project_name(),
    description : 'A message broker library for SRT Streaming service',
    filebase : chamge_api_name,
    subdirs: chamge_api_name,
    variables: 'exec_prefix=${prefix}'
  )

else

  pkg.generate(
    libraries : [ libchamge, libchamge_dbus ],
    version : meson.project_version(),
    name : meson.project_name(),
    description : 'A message broker library for SRT Streaming service',
    filebase : chamge_api_name,
    requires : [
      'librabbitmq',
      'json-glib-1.0',
    ],
    requires_private : [
      'glib-2.0 ' + glib_req_version,
      'gobject-2.0 ' + glib_req_version,
      'gio-2.0 ' + glib_req_version,
    ],
    subdirs: chamge_api_name,
    variables: 'exec_prefix=${prefix}'
  )

endif

