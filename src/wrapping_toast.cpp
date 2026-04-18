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

#include "wrapping_toast.hpp"

struct _WrappingToast {
  GtkBox parent_instance;

  GtkRevealer *revealer;

  GtkLabel *label;

  GtkButton *close_button;
};

G_DEFINE_TYPE(WrappingToast, wrapping_toast, GTK_TYPE_BOX)

static void on_close_button_clicked(GtkButton *, gpointer user_data) {
  auto toast = CBWAITA_WRAPPING_TOAST(user_data);

  gtk_revealer_set_reveal_child(GTK_REVEALER(toast->revealer), 0);
}

static void wrapping_toast_dispose(GObject *object) {
  G_OBJECT_CLASS(wrapping_toast_parent_class)->dispose(object);

  g_debug("Wrapping toast disposed.");
}

static void wrapping_toast_class_init(WrappingToastClass *klass) {
  auto object_class = G_OBJECT_CLASS(klass);
  auto widget_class = GTK_WIDGET_CLASS(klass);

  object_class->dispose = wrapping_toast_dispose;

  gtk_widget_class_set_template_from_resource(
      widget_class, "/com/github/digitalone1/clipboardwaita/wrapping_toast.ui");

  gtk_widget_class_bind_template_child(widget_class, WrappingToast, revealer);

  gtk_widget_class_bind_template_child(widget_class, WrappingToast, label);

  gtk_widget_class_bind_template_child(widget_class, WrappingToast,
                                       close_button);
}

static void wrapping_toast_init(WrappingToast *self) {
  gtk_widget_init_template(GTK_WIDGET(self));

  // Start with releaver hidden.
  gtk_revealer_set_reveal_child(self->revealer, 0);

  g_signal_connect(self->close_button, "clicked",
                   G_CALLBACK(on_close_button_clicked), self);

  g_debug("Wrapping toast initialized.");
}

auto wrapping_toast_new(void) -> WrappingToast * {
  return CBWAITA_WRAPPING_TOAST(
      g_object_new(CBWAITA_TYPE_WRAPPING_TOAST, nullptr));
}

void wrapping_toast_set_text(WrappingToast *toast, const std::string &text) {
  gtk_label_set_text(toast->label, text.c_str());
}

void wrapping_toast_show(WrappingToast *toast) {
  gtk_revealer_set_reveal_child(GTK_REVEALER(toast->revealer), 1);
}
