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

#include "clipboard_item_row.hpp"
#include "cbwaita_app.hpp"
#include "clipboard_item_holder.hpp"
#include "clipboard_list_manager.hpp"
#include "clipboard_watcher.hpp"
#include "edit_item_dialog.hpp"
#include "util.hpp"

constexpr uint max_item_label_length = 200U;

enum { PROP_0, PROP_ITEM_HOLDER, N_PROPERTIES };

static GParamSpec *properties[N_PROPERTIES] = {nullptr};

struct _ItemRow {
  GtkBox parent_instance;

  GtkButton *item_copy_content_button;

  GtkLabel *datetime_label, *text_content_label;

  GtkBox *action_right_container, *action_right_box;

  GtkMenuButton *action_right_button_mobile;

  GtkButton *item_reset_button, *item_edit_button, *item_remove_button,
      *item_reset_button_mobile, *item_edit_button_mobile,
      *item_remove_button_mobile;

  ItemHolder *item_holder;

  gulong breakpoint_apply_handler = 0UL, breakpoint_unapply_handler = 0UL;
};

G_DEFINE_TYPE(ItemRow, item_row, GTK_TYPE_BOX)

static void copy_item_content_to_clipboard(GtkButton *, gpointer user_data) {
  auto clipboard = ClipboardWatcher::get_gdk_clipboard();

  if (clipboard == nullptr) {
    return;
  }

  auto item_row = CBWAITA_ITEM_ROW(user_data);

  auto model_item = item_holder_get_data(item_row->item_holder);

  auto content = model_item->hash == model_item->hash_modified
                     ? model_item->text
                     : model_item->text_modified;

  // g_debug("Copy \"%s\" to the clipboard.", content);

  // Copy content to clipboard.
  gdk_clipboard_set_text(clipboard, content);

  auto settings = CbwaitaApp::get_settings();
  auto app_window = CbwaitaApp::get_window();

  if (app_window == nullptr) {
    return;
  }

  if (!g_settings_get_boolean(settings, "background-service-mode") ||
      !g_settings_get_boolean(settings, "autoclose-on-copy")) {

    return;
  }

  // In background service, close the window if the related setting is enabled.
  gtk_window_close(GTK_WINDOW(app_window));
}

static void item_row_set_date_time_label(ItemRow *item_row,
                                         ClipboardModelItem *model_item) {
  static constexpr auto bold_format = "<span weight=\"bold\">\%s</span>";

  std::string date_time_markup;

  g_autoptr(GDateTime) gdate_original =
      g_date_time_new_from_unix_local(model_item->timestamp);

  if (gdate_original == nullptr) {
    g_warning_once("Cannot format GDateTime from %s original timestamp.",
                   Util::to_string(model_item->timestamp).c_str());

    return;
  }

  g_autofree gchar *date_time_format_original =
      g_date_time_format(gdate_original, "%X %x");

  date_time_markup = std::string("Added at ") + bold_format;

  const auto item_modified = model_item->hash != model_item->hash_modified;

  if (!item_modified) {
    g_autofree char *escaped_markup = g_markup_printf_escaped(
        date_time_markup.c_str(), date_time_format_original);

    gtk_label_set_markup(item_row->datetime_label, escaped_markup);

    return;
  }

  g_autoptr(GDateTime) gdate_modified =
      g_date_time_new_from_unix_local(model_item->timestamp_modified);

  if (gdate_modified == nullptr) {
    g_warning_once("Cannot format GDateTime from %s modified timestamp.",
                   Util::to_string(model_item->timestamp_modified).c_str());

    return;
  }

  g_autofree gchar *date_time_format_modified =
      g_date_time_format(gdate_modified, "%X %x");

  date_time_markup += std::string(" · Modified at ") + bold_format;

  g_autofree char *escaped_markup = g_markup_printf_escaped(
      date_time_markup.c_str(), date_time_format_original,
      date_time_format_modified);

  gtk_label_set_markup(item_row->datetime_label, escaped_markup);
}

static void on_item_edit_button_clicked(GtkButton *, gpointer user_data) {
  // We do not have to unreference the dialog, otherwise it won't be shown.
  // It's automatically disposed when closed.
  auto edit_item_dialog =
      ADW_DIALOG(edit_item_dialog_new(CBWAITA_ITEM_ROW(user_data)));

  auto window_widget = GTK_WIDGET(CbwaitaApp::get_window());

  adw_dialog_present(edit_item_dialog, window_widget);
}

static void on_item_reset_button_clicked(GtkButton *self, gpointer user_data) {
  // We disable the button to avoid a double click.
  gtk_widget_set_sensitive(GTK_WIDGET(self), 0);

  auto item_row = CBWAITA_ITEM_ROW(user_data);

  auto item_holder = CBWAITA_ITEM_HOLDER(item_row_get_holder(item_row));

  auto model_item = item_holder_get_data(item_holder);

  ClipboardListManager::update_model_item(model_item, model_item->text);

  // Update the item row UI.
  item_row_update_widgets_state(item_row);
}

/**
 * Callback of the clicked signal to remove a single item.
 * In order to avoid to define another callback to remove a single item, we
 * reuse the existing one to remove multiple items just creating a new GList
 * and filling it with the parent of the self object.
 */
static void on_item_remove_button_clicked(GtkButton *self, gpointer user_data) {
  // We disable the button to avoid a double click.
  gtk_widget_set_sensitive(GTK_WIDGET(self), 0);

  auto item_row = CBWAITA_ITEM_ROW(user_data);

  auto list_bow_row_container = gtk_widget_get_parent(GTK_WIDGET(item_row));

  // GList is built incrementally filling it with new nodes and should always be
  // unreferenced when not needed.
  GList *list = g_list_prepend(nullptr, list_bow_row_container);

  // Pass ownership of list.
  g_idle_add_once(ClipboardListManager::list_model_remove_selected_items, list);
}

static void on_apply_window_breakpoint(AdwBreakpoint *, gpointer user_data) {
  auto item_row = CBWAITA_ITEM_ROW(user_data);

  gtk_widget_set_visible(GTK_WIDGET(item_row->action_right_box), 0);
  gtk_widget_set_visible(GTK_WIDGET(item_row->action_right_button_mobile), 1);
}

static void on_unapply_window_breakpoint(AdwBreakpoint *, gpointer user_data) {
  auto item_row = CBWAITA_ITEM_ROW(user_data);

  gtk_widget_set_visible(GTK_WIDGET(item_row->action_right_button_mobile), 0);
  gtk_widget_set_visible(GTK_WIDGET(item_row->action_right_box), 1);
}

static void item_row_set_property(GObject *object, guint prop_id,
                                  const GValue *value, GParamSpec *pspec) {
  auto self = CBWAITA_ITEM_ROW(object);

  switch (prop_id) {
  case PROP_ITEM_HOLDER: {
    auto new_holder = CBWAITA_ITEM_HOLDER(g_value_get_object(value));

    if (self->item_holder == new_holder) {
      break;
    }

    if (self->item_holder != nullptr) {
      g_object_unref(self->item_holder);
    }

    self->item_holder = new_holder != nullptr
                            ? CBWAITA_ITEM_HOLDER(g_object_ref(new_holder))
                            : nullptr;

    break;
  }

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}

static void item_row_get_property(GObject *object, guint prop_id, GValue *value,
                                  GParamSpec *pspec) {
  auto self = CBWAITA_ITEM_ROW(object);

  switch (prop_id) {
  case PROP_ITEM_HOLDER:
    g_value_set_object(value, self->item_holder);

    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}

static void item_row_dispose(GObject *object) {
  auto self = CBWAITA_ITEM_ROW(object);

  g_clear_object(&self->item_holder);

  auto app_window = CbwaitaApp::get_window();

  // Disconnect window breakpoint handlers
  if (app_window != nullptr) {
    auto window_breakpoint = app_window_get_breakpoint(app_window);

    if (self->breakpoint_apply_handler != 0UL) {
      g_signal_handler_disconnect(window_breakpoint,
                                  self->breakpoint_apply_handler);
      self->breakpoint_apply_handler = 0UL;
    }

    if (self->breakpoint_unapply_handler != 0UL) {
      g_signal_handler_disconnect(window_breakpoint,
                                  self->breakpoint_unapply_handler);
      self->breakpoint_unapply_handler = 0UL;
    }
  }

  G_OBJECT_CLASS(item_row_parent_class)->dispose(object);

  g_debug("Item row disposed.");
}

static void item_row_constructed(GObject *object) {
  auto self = CBWAITA_ITEM_ROW(object);

  G_OBJECT_CLASS(item_row_parent_class)->constructed(object);

  if (self->item_holder == nullptr) {
    return;
  }

  // Update row UI state
  item_row_update_widgets_state(self);

  // Left actions
  g_signal_connect(self->item_copy_content_button, "clicked",
                   G_CALLBACK(copy_item_content_to_clipboard), self);

  // Right actions
  g_signal_connect(self->item_reset_button, "clicked",
                   G_CALLBACK(on_item_reset_button_clicked), self);
  g_signal_connect(self->item_reset_button_mobile, "clicked",
                   G_CALLBACK(on_item_reset_button_clicked), self);

  g_signal_connect(self->item_edit_button, "clicked",
                   G_CALLBACK(on_item_edit_button_clicked), self);
  g_signal_connect(self->item_edit_button_mobile, "clicked",
                   G_CALLBACK(on_item_edit_button_clicked), self);

  g_signal_connect(self->item_remove_button, "clicked",
                   G_CALLBACK(on_item_remove_button_clicked), self);
  g_signal_connect(self->item_remove_button_mobile, "clicked",
                   G_CALLBACK(on_item_remove_button_clicked), self);

  auto app_window = CbwaitaApp::get_window();

  if (app_window != nullptr) {
    // Connect window breakpoint signals
    auto window_breakpoint = app_window_get_breakpoint(app_window);

    self->breakpoint_apply_handler =
        g_signal_connect(window_breakpoint, "apply",
                         G_CALLBACK(on_apply_window_breakpoint), self);
    self->breakpoint_unapply_handler =
        g_signal_connect(window_breakpoint, "unapply",
                         G_CALLBACK(on_unapply_window_breakpoint), self);
  }

  // Bind selection mode
  auto selection_mode_toggle_button =
      app_window != nullptr ? app_window_get_selection_mode_button(app_window)
                            : nullptr;

  if (selection_mode_toggle_button != nullptr) {
    g_object_bind_property(selection_mode_toggle_button, "active",
                           self->action_right_container, "visible",
                           G_BINDING_INVERT_BOOLEAN | G_BINDING_SYNC_CREATE);
  }

  g_debug("Item row constructed.");
}

static void item_row_class_init(ItemRowClass *klass) {
  auto object_class = G_OBJECT_CLASS(klass);
  auto widget_class = GTK_WIDGET_CLASS(klass);

  object_class->set_property = item_row_set_property;
  object_class->get_property = item_row_get_property;
  object_class->constructed = item_row_constructed;
  object_class->dispose = item_row_dispose;

  properties[PROP_ITEM_HOLDER] = g_param_spec_object(
      "item-holder", "Item Holder", "Clipboard item holder object",
      CBWAITA_TYPE_ITEM_HOLDER, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties(object_class, N_PROPERTIES, properties);

  gtk_widget_class_set_template_from_resource(
      widget_class,
      "/com/github/digitalone1/clipboardwaita/clipboard_list_row.ui");

  gtk_widget_class_bind_template_child(widget_class, ItemRow,
                                       item_copy_content_button);

  gtk_widget_class_bind_template_child(widget_class, ItemRow, datetime_label);
  gtk_widget_class_bind_template_child(widget_class, ItemRow,
                                       text_content_label);

  gtk_widget_class_bind_template_child(widget_class, ItemRow,
                                       action_right_container);
  gtk_widget_class_bind_template_child(widget_class, ItemRow, action_right_box);
  gtk_widget_class_bind_template_child(widget_class, ItemRow,
                                       action_right_button_mobile);

  gtk_widget_class_bind_template_child(widget_class, ItemRow,
                                       item_reset_button);
  gtk_widget_class_bind_template_child(widget_class, ItemRow, item_edit_button);
  gtk_widget_class_bind_template_child(widget_class, ItemRow,
                                       item_remove_button);
  gtk_widget_class_bind_template_child(widget_class, ItemRow,
                                       item_reset_button_mobile);
  gtk_widget_class_bind_template_child(widget_class, ItemRow,
                                       item_edit_button_mobile);
  gtk_widget_class_bind_template_child(widget_class, ItemRow,
                                       item_remove_button_mobile);

  g_debug("Item row class initialized.");
}

static void item_row_init(ItemRow *self) {
  gtk_widget_init_template(GTK_WIDGET(self));

  g_debug("Item row initialized.");
}

auto item_row_new(GObject *item_holder) -> ItemRow * {
  return CBWAITA_ITEM_ROW(
      g_object_new(CBWAITA_TYPE_ITEM_ROW, "item-holder", item_holder, nullptr));
}

auto item_row_get_holder(ItemRow *item_row) -> ItemHolder * {
  return item_row->item_holder;
}

void item_row_update_widgets_state(ItemRow *item_row) {
  auto model_item = item_holder_get_data(item_row->item_holder);

  const auto trimmed_string = Util::trim_string_view(model_item->text_modified);

  // Content label
  if (trimmed_string.empty()) {
    static constexpr auto label_text = "Empty spaces";
    static constexpr auto italic_format = "<span style=\"italic\">\%s</span>";

    g_autofree char *escaped_markup =
        g_markup_printf_escaped(italic_format, label_text);

    gtk_label_set_markup(item_row->text_content_label, escaped_markup);
  } else {
    const auto sane_label = Util::trim_single_space_string(
        model_item->text_modified, max_item_label_length);

    gtk_label_set_text(item_row->text_content_label, sane_label.c_str());
  }

  // Date-time label
  item_row_set_date_time_label(item_row, model_item);

  // Set right action boxes visibility upon applied breakpoint state
  auto app_window = CbwaitaApp::get_window();

  if (app_window != nullptr) {
    const auto window_breakpoint_applied =
        app_window_breakpoint_applied(app_window);

    gtk_widget_set_visible(GTK_WIDGET(item_row->action_right_box),
                           !window_breakpoint_applied);
    gtk_widget_set_visible(GTK_WIDGET(item_row->action_right_button_mobile),
                           window_breakpoint_applied);
  }

  // Reset button
  auto reset_button_widget = GTK_WIDGET(item_row->item_reset_button);
  auto reset_button_widget_mobile =
      GTK_WIDGET(item_row->item_reset_button_mobile);

  const auto item_modified = model_item->hash != model_item->hash_modified;

  gtk_widget_set_visible(reset_button_widget, item_modified);
  gtk_widget_set_sensitive(reset_button_widget, item_modified);
  gtk_widget_set_visible(reset_button_widget_mobile, item_modified);
  gtk_widget_set_sensitive(reset_button_widget_mobile, item_modified);
}
