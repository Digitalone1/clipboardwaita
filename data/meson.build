gnome.compile_schemas(
  depend_files: 'com.github.digitalone1.clipboardwaita.gschema.xml',
  build_by_default: true
)

install_data(
  'com.github.digitalone1.clipboardwaita.gschema.xml',
  install_dir: get_option('datadir') / 'glib-2.0/schemas',
)

install_data(
  'com.github.digitalone1.clipboardwaita.desktop',
  install_dir: get_option('datadir') / 'applications'
)

install_data(
  'icons/com.github.digitalone1.clipboardwaita.svg',
  install_dir: get_option('datadir') / 'icons/hicolor/scalable/apps'
)

install_data(
  'com.github.digitalone1.clipboardwaita.xml',
  install_dir: get_option('datadir') / 'metainfo'
)
