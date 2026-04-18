/**
 * Copyright © 2026-2027 Digitalone1
 *
 * This file is part of Clipboard History.
 *
 * Clipboard History is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Clipboard History is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Clipboard History. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib-object.h>
#include <gtk/gtk.h>
#include <string>

/**
 * Since Libadwaita does not provide a toast that wraps text in multiple
 * lines, we are forced to do a similar widget.
 * Wrapping toast are used to show long messages to do user without a
 * autohiding timeout (the message remains visible and the user should close
 * it).
 */

G_BEGIN_DECLS

#define CBWAITA_TYPE_WRAPPING_TOAST (wrapping_toast_get_type())

G_DECLARE_FINAL_TYPE(WrappingToast, wrapping_toast, CBWAITA, WRAPPING_TOAST,
                     GtkBox)

G_END_DECLS

auto wrapping_toast_new() -> WrappingToast *;

void wrapping_toast_set_text(WrappingToast *toast, const std::string &text);

void wrapping_toast_show(WrappingToast *toast);
