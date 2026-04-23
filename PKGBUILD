pkgname=clipboardwaita-git
pkgver=r0.0
pkgrel=1
pkgdesc="Clipboard history manager built with Libadwaita"
arch=(x86_64)
url="https://github.com/Digitalone1/clipboardwaita"
license=(GPL-3.0-or-later)

depends=(
  glib2
  glibc
  gsettings-desktop-schemas
  gtk4
  hicolor-icon-theme
  libadwaita
  libgcc
)

makedepends=(
  meson
  ninja
  gcc
  pkgconf
  git
)

conflicts=("${pkgname%%-git}")
provides=("${pkgname%%-git}")

source=("${pkgname%%-git}::git+${url}")
sha512sums=('SKIP')

pkgver() {
  cd "${pkgname%%-git}"

  git describe --long | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g'
}

build() {
  cd "${pkgname%%-git}"

  meson setup build --prefix=/usr

  ninja -C build
}

package() {
  cd "${pkgname%%-git}"

  DESTDIR="${pkgdir}" ninja -C build install
}
