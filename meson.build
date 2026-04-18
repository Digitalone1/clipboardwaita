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

adw = dependency('libadwaita-1', required: true, include_type: 'system')
gtk = dependency('gtk4', required: true, include_type: 'system')
glib = dependency('glib-2.0', required: true, include_type: 'system')
gio = dependency('gio-2.0', required: true, include_type: 'system')
gobject = dependency('gobject-2.0', required: true, include_type: 'system')
gthread = dependency('gthread-2.0', required: false, include_type: 'system')

subdir('src')
subdir('data')
