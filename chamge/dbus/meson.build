chamge_dbus_install_header_subdir = join_paths(chamge_install_header_subdir, 'dbus')

sources = [
]

dbus_ifaces = [
  [ 'Edge.Manager', 'edge-manager-generated' ]
]

foreach iface: dbus_ifaces
  prefix = 'org.hwangsaeul.Chamge1.'
  name = prefix + iface[0]

  if meson.version().version_compare('>= 0.46.0')
    sources += gnome.gdbus_codegen(
      iface[1],
      sources: name + '.xml',
      interface_prefix: prefix,
      namespace: 'ChamgeDBus',
    )
  else
    sources += custom_target(
      iface[1],
      input: name + '.xml',
      output: [iface[1] + '.h', iface[1] + '.c'],
      command: [
        find_program('codegen.py'), iface[0], iface[1],
        meson.current_build_dir(), '@INPUT@'
      ]
    )
  endif
endforeach

libchamge_dbus = shared_library(
  'chamge-dbus-@0@'.format(apiversion),
  sources,
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
