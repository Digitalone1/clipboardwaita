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

#include "preferences_dialog.hpp"
#include "util.hpp"

// Pointers marked as NULL should be explicitly initialized in init.
struct _PreferencesDialog {
  AdwPreferencesDialog parent_instance;

  AdwSwitchRow *clipboard_tracking_switch;

  AdwComboRow *new_duplicate_entry_comborow;

  GtkButton *apply_new_list_size_button;

  GtkSpinButton *list_size_spinbutton;

  GtkSwitch *background_service_switch;

  AdwSwitchRow *autoclose_on_copy_switch;

  GSettings *settings = nullptr;
};

G_DEFINE_TYPE(PreferencesDialog, preferences_dialog,
              ADW_TYPE_PREFERENCES_DIALOG)

/**
 * Apply warning style when the spinbutton shows a value different than the
 * current GSettings value.
 * The value is not applied to GSettings key here.
 */
static void spinbutton_list_size_value_changed(GtkSpinButton *self,
                                               gpointer user_data) {
  auto pref_dialog = CBWAITA_PREFERENCES_DIALOG(user_data);

  auto self_widget = GTK_WIDGET(self);

  auto apply_button = GTK_WIDGET(pref_dialog->apply_new_list_size_button);

  const auto spinbutton_value =
      static_cast<guint>(gtk_spin_button_get_value_as_int(self));

  const auto list_model_size =
      g_settings_get_uint(pref_dialog->settings, "clipboard-history-size");

  if (spinbutton_value == list_model_size) {
    gtk_widget_set_visible(apply_button, 0);
    gtk_widget_set_sensitive(apply_button, 0);

    gtk_widget_remove_css_class(self_widget, "warning");

    return;
  }

  gtk_widget_set_visible(apply_button, 1);
  gtk_widget_set_sensitive(apply_button, 1);

  if (!gtk_widget_has_css_class(self_widget, "warning")) {
    gtk_widget_add_css_class(self_widget, "warning");
  }
}

/**
 * Apply the new list model size value to GSettings key.
 * This is a crucial callback because it triggers two subsequent signals:
 *
 * The first is the GSettings value changed signal, from which the related
 * callback will apply the new size to the list model (last items could be
 * removed from the list if necessary).
 *
 * The second is the GIO list model items-changed, from which the related
 * callback will update the list size indicator in the main window.
 */
static void apply_new_list_model_size(GtkButton *self, gpointer user_data) {
  auto self_widget = GTK_WIDGET(self);

  auto pref_dialog = CBWAITA_PREFERENCES_DIALOG(user_data);

  gtk_widget_set_visible(self_widget, 0);
  gtk_widget_set_sensitive(self_widget, 0);

  gtk_widget_remove_css_class(GTK_WIDGET(pref_dialog->list_size_spinbutton),
                              "warning");

  const auto current_list_size =
      g_settings_get_uint(pref_dialog->settings, "clipboard-history-size");

  const auto new_list_size = static_cast<guint>(
      gtk_spin_button_get_value_as_int(pref_dialog->list_size_spinbutton));

  if (current_list_size == new_list_size) {
    return;
  }

  g_debug("Apply new list model maximum size");

  // The new max size value on the list model will be applied by the callback
  // on changed signal of the following GSettings key:
  g_settings_set_uint(pref_dialog->settings, "clipboard-history-size",
                      new_list_size);
}

/**
 * Directly invoke the callback to apply the new list size value when the
 * spinbutton is activated (i.e. the user presses ENTER in the text field).
 */
static void spinbutton_list_size_value_activated(GtkSpinButton *,
                                                 gpointer user_data) {
  auto preferences_dialog = CBWAITA_PREFERENCES_DIALOG(user_data);

  // When ENTER is pressed on the spinbutton, just apply the value.
  apply_new_list_model_size(preferences_dialog->apply_new_list_size_button,
                            preferences_dialog);
}

static void preferences_dialog_dispose(GObject *object) {
  auto self = CBWAITA_PREFERENCES_DIALOG(object);

  g_clear_object(&self->settings);

  G_OBJECT_CLASS(preferences_dialog_parent_class)->dispose(object);

  g_debug("Preferences dialog disposed.");
}

static void preferences_dialog_class_init(PreferencesDialogClass *klass) {
  auto object_class = G_OBJECT_CLASS(klass);
  auto widget_class = GTK_WIDGET_CLASS(klass);

  object_class->dispose = preferences_dialog_dispose;

  gtk_widget_class_set_template_from_resource(
      widget_class,
      "/com/github/digitalone1/clipboardwaita/preferences_dialog.ui");

  gtk_widget_class_bind_template_child(widget_class, PreferencesDialog,
                                       clipboard_tracking_switch);

  gtk_widget_class_bind_template_child(widget_class, PreferencesDialog,
                                       new_duplicate_entry_comborow);

  gtk_widget_class_bind_template_child(widget_class, PreferencesDialog,
                                       apply_new_list_size_button);
  gtk_widget_class_bind_template_child(widget_class, PreferencesDialog,
                                       list_size_spinbutton);

  gtk_widget_class_bind_template_child(widget_class, PreferencesDialog,
                                       background_service_switch);
  gtk_widget_class_bind_template_child(widget_class, PreferencesDialog,
                                       autoclose_on_copy_switch);
}

static void preferences_dialog_init(PreferencesDialog *self) {
  gtk_widget_init_template(GTK_WIDGET(self));

  // Settings
  self->settings = g_settings_new("com.github.digitalone1.clipboardwaita");

  // Clipboard tracking

  // New duplicate entry combo row
  Util::g_settings_bind_combo_row_with_mapping(
      self->settings, "new-duplicate-entry",
      self->new_duplicate_entry_comborow);

  g_settings_bind(self->settings, "clipboard-tracking",
                  self->clipboard_tracking_switch, "active",
                  G_SETTINGS_BIND_DEFAULT);

  // List size
  preferences_dialog_restore(self);

  g_signal_connect(self->list_size_spinbutton, "value-changed",
                   G_CALLBACK(spinbutton_list_size_value_changed), self);

  g_signal_connect(self->list_size_spinbutton, "activate",
                   G_CALLBACK(spinbutton_list_size_value_activated), self);

  g_signal_connect(self->apply_new_list_size_button, "clicked",
                   G_CALLBACK(apply_new_list_model_size), self);

  // Background service
  g_settings_bind(self->settings, "background-service-mode",
                  self->background_service_switch, "active",
                  G_SETTINGS_BIND_DEFAULT);

  // Autoclose on copy
  g_settings_bind(self->settings, "autoclose-on-copy",
                  self->autoclose_on_copy_switch, "active",
                  G_SETTINGS_BIND_DEFAULT);
}

auto preferences_dialog_new() -> PreferencesDialog * {
  return CBWAITA_PREFERENCES_DIALOG(
      g_object_new(CBWAITA_TYPE_PREFERENCES_DIALOG, nullptr));
}

void preferences_dialog_restore(PreferencesDialog *pref_dialog) {
  const auto list_model_size = static_cast<gdouble>(
      g_settings_get_uint(pref_dialog->settings, "clipboard-history-size"));

  gtk_spin_button_set_value(pref_dialog->list_size_spinbutton, list_model_size);

  g_debug("Preferences dialog restored.");
}
