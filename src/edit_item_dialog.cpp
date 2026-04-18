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

#include "edit_item_dialog.hpp"
#include "app_window_ui.hpp"
#include "cbwaita_app.hpp"
#include "clipboard_item_holder.hpp"
#include "clipboard_item_row.hpp"
#include "clipboard_list_manager.hpp"
#include "util.hpp"

enum { PROP_0, PROP_ITEM_ROW, N_PROPERTIES };

static GParamSpec *properties[N_PROPERTIES] = {nullptr};

// Item row is weak referenced. New text is not owned.
struct _EditItemDialog {
  AdwDialog parent_instance;

  GtkTextBuffer *item_buffer;

  GtkLabel *info_label;

  GtkButton *edit_save_text;

  char *new_text = nullptr;

  ItemRow *item_row = nullptr;
};

G_DEFINE_TYPE(EditItemDialog, edit_item_dialog, ADW_TYPE_DIALOG)

/**
 * This callback is invoked by g_idle_once to save the new item text.
 * Here we assume the new_text has been set to point to a valid c-string.
 * Even if "new_text" of EditItemDialog is declared as not owned, the pointed
 * string is owned in this context, but the owneship is passed to the model
 * item (if the item_row is alive).
 */
static void save_new_item_text(gpointer user_data) {
  auto dialog = CBWAITA_EDIT_ITEM_DIALOG(user_data);

  if (dialog->item_row == nullptr) {
    g_debug("Item row has been already disposed.");

    Util::g_free_string(dialog->new_text);

    // Close the dialog.
    dialog->new_text = nullptr;
    adw_dialog_close(ADW_DIALOG(dialog));

    auto app_window = CbwaitaApp::get_window();

    if (app_window == nullptr) {
      return;
    }

    // Show the overlay toast to inform the user.
    app_window_show_wrapping_toast(
        CbwaitaApp::get_window(),
        "The entry has not been modified because it was deleted. This likely "
        "happens when you copy the same content and \"Remove old duplicate\" "
        "option is set or the entry exceeded the maximum list size while new "
        "ones have been inserted.");

    return;
  }

  auto item_row = dialog->item_row;

  auto item_holder = CBWAITA_ITEM_HOLDER(item_row_get_holder(item_row));

  auto model_item = item_holder_get_data(item_holder);

  static constexpr std::hash<std::string_view> str_hash;

  if (str_hash(dialog->new_text) == model_item->hash_modified) {
    // The new text has not been modified, but the user. Just close the dialog.
    g_debug("Item content has no been modified.");

    Util::g_free_string(dialog->new_text);

    dialog->new_text = nullptr;
    adw_dialog_close(ADW_DIALOG(dialog));

    return;
  }

  // Pass ownership to local pointer variable.
  auto new_text = dialog->new_text;

  /**
   * Since we have moved the ownership of the new text in the local variable,
   * we can reset "new_text" of the EditItemDialog to NULL and close/dispose
   * the dialog since we don't need it anymore.
   */
  dialog->new_text = nullptr;
  adw_dialog_close(ADW_DIALOG(dialog));

  // Update the list model.
  ClipboardListManager::update_model_item(model_item, new_text);

  // Update the item row UI.
  item_row_update_widgets_state(item_row);
}

static void on_save_button_clicked(GtkButton *self, gpointer user_data) {
  gtk_widget_set_sensitive(GTK_WIDGET(self), 0);

  auto dialog = CBWAITA_EDIT_ITEM_DIALOG(user_data);

  gtk_widget_set_visible(GTK_WIDGET(dialog->info_label), 0);

  GtkTextIter start, end;

  gtk_text_buffer_get_bounds(dialog->item_buffer, &start, &end);

  dialog->new_text =
      gtk_text_buffer_get_text(dialog->item_buffer, &start, &end, 0);

  constexpr std::string_view empty_str_view;

  // We do not allow empty strings in the item text content.
  if (dialog->new_text == nullptr ||
      std::string_view(dialog->new_text) == empty_str_view) {
    Util::g_free_string(dialog->new_text);

    dialog->new_text = nullptr;

    gtk_label_set_text(
        dialog->info_label,
        "The text is empty. Write something to update the entry.");

    gtk_widget_set_visible(GTK_WIDGET(dialog->info_label), 1);
    gtk_widget_set_sensitive(GTK_WIDGET(self), 1);

    return;
  }

  /**
   * If the item row is deleted, it's quite safe here to check if the weak
   * pointer has been set to NULL, but to be even safer we make the update
   * operations in the g_idle_add_once callback.
   * The dialog will be closed there.
   */
  g_idle_add_once(save_new_item_text, dialog);
}

static void edit_item_dialog_set_property(GObject *object, guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec) {
  auto self = CBWAITA_EDIT_ITEM_DIALOG(object);

  switch (prop_id) {
  case PROP_ITEM_ROW:
    g_set_weak_pointer(&self->item_row,
                       CBWAITA_ITEM_ROW(g_value_get_object(value)));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}

static void edit_item_dialog_get_property(GObject *object, guint prop_id,
                                          GValue *value, GParamSpec *pspec) {
  auto self = CBWAITA_EDIT_ITEM_DIALOG(object);

  switch (prop_id) {
  case PROP_ITEM_ROW:
    g_value_set_object(value, self->item_row);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}

static void edit_item_dialog_dispose(GObject *object) {
  auto self = CBWAITA_EDIT_ITEM_DIALOG(object);

  if (self->item_row) {
    g_set_weak_pointer(&self->item_row, nullptr);
  }

  G_OBJECT_CLASS(edit_item_dialog_parent_class)->dispose(object);

  g_debug("Edit item dialog disposed.");
}

static void edit_item_dialog_constructed(GObject *object) {
  auto self = CBWAITA_EDIT_ITEM_DIALOG(object);

  G_OBJECT_CLASS(edit_item_dialog_parent_class)->constructed(object);

  if (self->item_row == nullptr) {
    return;
  }

  auto item_holder = item_row_get_holder(self->item_row);

  auto model_item = item_holder_get_data(item_holder);

  gtk_text_buffer_set_text(self->item_buffer, model_item->text_modified, -1);

  g_signal_connect(self->edit_save_text, "clicked",
                   G_CALLBACK(on_save_button_clicked), self);

  g_debug("Edit item dialog class constructed.");
}

static void edit_item_dialog_class_init(EditItemDialogClass *klass) {
  auto object_class = G_OBJECT_CLASS(klass);
  auto widget_class = GTK_WIDGET_CLASS(klass);

  object_class->set_property = edit_item_dialog_set_property;
  object_class->get_property = edit_item_dialog_get_property;
  object_class->constructed = edit_item_dialog_constructed;
  object_class->dispose = edit_item_dialog_dispose;

  properties[PROP_ITEM_ROW] = g_param_spec_object(
      "item-row", "Item Row", "Clipboard list item row object",
      CBWAITA_TYPE_ITEM_ROW, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties(object_class, N_PROPERTIES, properties);

  gtk_widget_class_set_template_from_resource(
      widget_class,
      "/com/github/digitalone1/clipboardwaita/edit_item_dialog.ui");

  gtk_widget_class_bind_template_child(widget_class, EditItemDialog,
                                       item_buffer);

  gtk_widget_class_bind_template_child(widget_class, EditItemDialog,
                                       info_label);

  gtk_widget_class_bind_template_child(widget_class, EditItemDialog,
                                       edit_save_text);
}

static void edit_item_dialog_init(EditItemDialog *self) {
  gtk_widget_init_template(GTK_WIDGET(self));
}

auto edit_item_dialog_new(ItemRow *item_row) -> EditItemDialog * {
  return CBWAITA_EDIT_ITEM_DIALOG(g_object_new(CBWAITA_TYPE_EDIT_ITEM_DIALOG,
                                               "item-row", item_row, nullptr));
}
