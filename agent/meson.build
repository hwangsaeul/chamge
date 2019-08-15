source_h = [
  'agent.h',
]

source_c = [
  'agent.c',
]

chamge_agent_c_args = [
  '-DG_LOG_DOMAIN="CHAMGE-AGENT"',
  '-DCHAMGE_COMPILATION',
]


chamge_agent = executable (
  'chamge-agent',
  source_c,
  include_directories: chamge_incs,
  c_args: chamge_agent_c_args,
  dependencies: [ gobject_dep, gio_dep, libchamge_dbus_dep, libchamge_dep ],
  install: true
)
