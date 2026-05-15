# Clipboardwaita

A **Clipboard Hystory** manager built with Libadwaita.

## Features

- Collect clipboard text data in background and show the history.
- Pin and/or modify entries.
- Easily search and select the preferred entries.
- Run the app as a background service and toggle the window with the related command line option.

## Unsupported features

- Images - The app is mainly focused on text data.
- Primary selection - May be added in the future, but it's not in the interests of the main developer.
- XDG Global Shortcuts - Not worth to add it, use command line options and bind them to you favorite keys.
- Flatpak - Main developer uses a custom PKGBUILD on Arch Linux. Contribution is welcome to add Flatpak support.

## Gnome limitations

Under Mutter Wayland compositor, the app needs to be focused to collect clipboard data. To workaround this limitation, run the app with XWayland:

```shell
GDK_BACKEND=x11 clipboardwaita
```

To launch with XWayland from the applications menu, copy the desktop entry from `/usr/share/applications` to `~/.config/autostart` and modify the `Exec` key:

```shell
Exec=env GDK_BACKEND=x11 clipboardwaita
```

## Installation

```shell
meson setup build

ninja -C build --prefix=/usr

sudo ninja -C build install
```

### Dependencies

- Libadwaita
- GTK4
