agents = [
  'edge-agent',
  'arbiter-agent',
]

chamge_agent_c_args = [
  '-DCHAMGE_COMPILATION',
]

foreach agent: agents
  exe_name = 'chamge-@0@'.format(agent)
  src_file = '@0@.c'.format(agent)

  executable(exe_name,
    src_file,
    install: true,
    include_directories: chamge_incs,
    c_args: [ chamge_agent_c_args, '-DG_LOG_DOMAIN="CHAMGE-ARBITER"' ],
    dependencies: [ libchamge_dbus_dep, libchamge_dep, json_glib_dep ],
  )

endforeach
