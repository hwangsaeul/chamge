source_c = [
  'glib-compat.c',
]

libglib_compat = static_library('chamge-glib-compat', source_c,
  dependencies: glib_dep)

glib_compat_dep = declare_dependency(link_with: libglib_compat)

glib_dep = [ glib_dep, glib_compat_dep ]
gio_dep = [ gio_dep, glib_compat_dep ]
gobject_dep = [ gobject_dep, glib_compat_dep ]
