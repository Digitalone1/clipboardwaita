project(
  'clipboardwaita',
  'cpp',
  version: '0.1',
  default_options: [
    'cpp_std=c++20',
    'warning_level=2'
  ]
)

gnome = import('gnome')

adw = dependency('libadwaita-1', required: true)
gtk = dependency('gtk4', required: true)
glib = dependency('glib-2.0', required: true)
gio = dependency('gio-2.0', required: true)
gobject = dependency('gobject-2.0', required: true)

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: true
)

subdir('src')
subdir('data')
