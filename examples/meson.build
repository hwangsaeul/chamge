edge_dummy_source_c = [
  'edge-dummy.c',
]

hub_dummy_source_c = [
  'hub-dummy.c',
]

arbiter_dummy_source_c = [
  'arbiter-dummy.c',
]

chamge_example_c_args = [
  '-DCHAMGE_COMPILATION',
]

edge_dummy = executable (
  'edge-dummy',
  edge_dummy_source_c,
  include_directories: chamge_incs,
  c_args: [ chamge_example_c_args, '-DG_LOG_DOMAIN="CHAMGE-EDGE"' ],
  dependencies: [ gobject_dep, gio_dep, libchamge_dbus_dep, libchamge_dep ],
  install: false
)

hub_dummy = executable (
  'hub-dummy',
  hub_dummy_source_c,
  include_directories: chamge_incs,
  c_args: [ chamge_example_c_args, '-DG_LOG_DOMAIN="CHAMGE-HUB"' ],
  dependencies: [ gobject_dep, gio_dep, libchamge_dbus_dep, libchamge_dep ],
  install: false
)

arbiter_dummy = executable (
  'arbiter-dummy',
  arbiter_dummy_source_c,
  include_directories: chamge_incs,
  c_args: [ chamge_example_c_args, '-DG_LOG_DOMAIN="CHAMGE-EDGE"' ],
  dependencies: [ gobject_dep, gio_dep, libchamge_dbus_dep, libchamge_dep ],
  install: false
)
