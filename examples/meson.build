dummys = [
  'edge-dummy',
  'hub-dummy',
  'arbiter-dummy',
]

chamge_example_c_args = [
  '-DCHAMGE_COMPILATION',
]

foreach dummy: dummys
  exe_name = '@0@'.format(dummy)
  src_file = '@0@.c'.format(dummy)

  executable(exe_name,
    src_file,
    include_directories: chamge_incs,
    c_args: [ chamge_example_c_args, '-DG_LOG_DOMAIN="CHAMGE-EDGE"' ],
    dependencies: [ gobject_dep, gio_dep, libchamge_dbus_dep, libchamge_dep ],
  install: false
  )
endforeach
