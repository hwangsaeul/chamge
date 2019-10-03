chamge_dbus_install_header_subdir = join_paths(chamge_install_header_subdir, 'dbus')

sources = [
]

dbus_ifaces = [
  [ 'Edge.Manager', 'edge-manager-generated' ]
]

dbus_headers = []

if meson.version().version_compare('>= 0.46.0')

  foreach iface: dbus_ifaces
    prefix = 'org.hwangsaeul.Chamge1.'
    name = prefix + iface[0] 
    dbus_sources = gnome.gdbus_codegen(
      iface[1],
      sources: name + '.xml',
      interface_prefix: prefix,
      namespace: 'ChamgeDBus',
    )

    dbus_headers = dbus_sources[0]
    sources += dbus_sources[1]

  endforeach

else
  codegen = find_program('codegen.py')

  foreach iface: dbus_ifaces
    prefix = 'org.hwangsaeul.Chamge1.'
    name = prefix + iface[0] 
    dbus_sources = custom_target(
      iface[1],
      input: name + '.xml',
      output: [iface[1] + '.h', iface[1] + '.c'],
      command: [codegen, iface[0], iface[1], meson.current_build_dir(), '@INPUT@', '@OUTPUT@']
    )

    dbus_headers += dbus_sources[0]
    sources += dbus_sources[1]

  endforeach

endif

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
