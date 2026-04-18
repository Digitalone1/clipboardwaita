app_sources = [
  'app_window_ui.cpp',
  'app_window_ui.hpp',
  'cbwaita_app.cpp',
  'cbwaita_app.hpp',
  'clipboard_item_holder.cpp',
  'clipboard_item_holder.hpp',
  'clipboard_item_row.cpp',
  'clipboard_item_row.hpp',
  'clipboard_list_manager.cpp',
  'clipboard_list_manager.hpp',
  'clipboard_watcher.cpp',
  'clipboard_watcher.hpp',
  'edit_item_dialog.cpp',
  'edit_item_dialog.hpp',
  'main.cpp',
  'preferences_dialog.hpp',
  'preferences_dialog.cpp',
  'util.cpp',
  'util.hpp',
  'wrapping_toast.cpp',
  'wrapping_toast.hpp'
]

gresources = gnome.compile_resources(
  'clipboardwaita_gresource',
  'clipboardwaita.gresource.xml',
)

gnome.post_install(
  glib_compile_schemas: true
)

executable(
  'clipboardwaita',
  app_sources,
  gresources,
  dependencies: [
    adw,
    gtk,
    glib,
    gio,
    gobject,
    gthread
  ],
  install: false
)
