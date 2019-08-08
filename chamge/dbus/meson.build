chamge_dbus_install_header_subdir = join_paths(chamge_install_header_subdir, 'dbus')

sources = [
]

dbus_ifaces = [
  [ 'MQ1', 'mp1-generated' ]
]

dbus_headers = []

foreach iface: dbus_ifaces
  prefix = 'org.hwangsaeul.chamge.'
  name = prefix + iface[0] 
  dbus_sources = gnome.gdbus_codegen(
    iface[1],
    sources: name + '.xml',
    interface_prefix: prefix,
    namespace: 'ChamgaeDBus',
  )
  dbus_headers = dbus_sources[0]
  sources += dbus_sources[1]

endforeach

libchamge_dbus = shared_library(
  'chamge-dbus-@0@'.format(apiversion),
  sources, dbus_headers,
  version: libversion,
  soversion: soversion,
  include_directories: chamge_incs,
  dependencies: [ gio_dep ],
  c_args: chamge_c_args,
  link_args: common_ldflags,
  install: true
)

libchamge_dbus_dep = declare_dependency(link_with: libchamge_dbus,
  include_directories: [ chamge_incs ],
  dependencies: [ gio_dep, libchamge_dep ]
)
