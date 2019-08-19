source_h = [
  'edge-agent.h',
]

source_c = [
  'edge-agent.c',
]

chamge_agent_c_args = [
  '-DCHAMGE_COMPILATION',
]

chamge_edge_agent = executable (
  'chamge-edge-agent',
  source_c,
  include_directories: chamge_incs,
  c_args: [ chamge_agent_c_args, '-DG_LOG_DOMAIN="CHAMGE-EDGE"' ],
  dependencies: [ gobject_dep, gio_dep, libchamge_dbus_dep, libchamge_dep ],
  install: true
)
