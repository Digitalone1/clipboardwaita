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

#include "app_window_ui.hpp"
#include "cbwaita_app.hpp"
#include "clipboard_item_row.hpp"
#include "clipboard_list_manager.hpp"
#include "preferences_dialog.hpp"
#include "util.hpp"
#include "wrapping_toast.hpp"

// Custom CSS.
constexpr auto custom_css = R"(
.wrapping-toast {
  background-color: var(--dialog-bg-color);
  border-radius: var(--window-radius);
  border: 1px solid var(--sidebar-bg-color);
}
)";

// Pointers marked as NULL should be explicitly initialized in init.
struct _AppWindow {
  AdwWindow parent_instance;

  AdwBreakpoint *window_breakpoint;

  GtkSwitch *clipboard_watcher_switch;

  GtkButton *remove_selected_items_button, *select_all_items_button;

  GtkToggleButton *search_mode_toggle_button, *selection_mode_toggle_button;

  GMenuModel *primary_menu;

  GMenu *service_section = nullptr;

  PreferencesDialog *preferences_dialog = nullptr;

  GtkSearchBar *top_search_bar;

  GtkSearchEntry *search_entry;

  GtkOverlay *content_overlay;

  AdwToastOverlay *content_adwtoast_overlay;

  WrappingToast *wrapping_toast = nullptr;

  GtkListBox *clipboard_history_listbox;

  AdwStatusPage *list_status_page, *search_status_page;

  GtkActionBar *footer_mobile;

  GtkLabel *footer_list_size, *footer_list_size_mobile, *footer_bg_service,
      *footer_bg_service_mobile;

  GSettings *settings = nullptr;
};

G_DEFINE_TYPE(AppWindow, app_window, ADW_TYPE_WINDOW)

static void initialize_window_state(AppWindow *self) {
  auto window_gtk = GTK_WINDOW(self);

  const auto window_width = g_settings_get_int(self->settings, "window-width");
  const auto window_height =
      g_settings_get_int(self->settings, "window-height");
  const auto window_maximized =
      g_settings_get_boolean(self->settings, "window-maximized") != 0;

  gtk_window_set_default_size(window_gtk, window_width, window_height);

  if (window_maximized) {
    gtk_window_maximize(window_gtk);
  }

  g_debug("Window state initialized.");
}

static void save_window_state(AppWindow *self) {
  auto window_gtk = GTK_WINDOW(self);

  int window_width = -1, window_height = -1;

  gtk_window_get_default_size(window_gtk, &window_width, &window_height);

  const auto window_maximized = gtk_window_is_maximized(window_gtk);

  g_settings_set_int(self->settings, "window-width", window_width);
  g_settings_set_int(self->settings, "window-height", window_height);
  g_settings_set_boolean(self->settings, "window-maximized", window_maximized);

  g_debug("Window state saved.");
}

static auto create_list_item_row(GObject *item, gpointer *) -> GtkWidget * {
  return GTK_WIDGET(item_row_new(item));
}

static void on_remove_selected_items_button_clicked(GtkButton *,
                                                    gpointer user_data) {
  auto app_window = CBWAITA_APP_WINDOW(user_data);

  auto selected_rows =
      gtk_list_box_get_selected_rows(app_window->clipboard_history_listbox);

  if (selected_rows == nullptr) {
    return;
  }

  g_idle_add_once(ClipboardListManager::list_model_remove_selected_items,
                  selected_rows);
}

static void on_select_all_rows_clicked(GtkButton *, gpointer user_data) {
  auto app_window = CBWAITA_APP_WINDOW(user_data);

  gtk_list_box_select_all(app_window->clipboard_history_listbox);
}

static void switch_clipboard_listbox_selection_mode(GtkToggleButton *self,
                                                    gpointer user_data) {
  auto app_window = CBWAITA_APP_WINDOW(user_data);

  const auto selection_mode = gtk_toggle_button_get_active(self)
                                  ? GTK_SELECTION_MULTIPLE
                                  : GTK_SELECTION_NONE;

  gtk_list_box_set_selection_mode(app_window->clipboard_history_listbox,
                                  selection_mode);
}

static void search_entry_text_changed(GtkSearchEntry *self, gpointer) {
  /**
   * The returned string is owned by the instance.
   * We have to remember also that the search_changed signal is emitted even
   * when the search bar is hidden and the search entry is automatically
   * emptied.
   */
  auto search_field_text = gtk_editable_get_text(GTK_EDITABLE(self));

  /**
   * We want to avoid sending empty strings or strings containing all
   * whitespaces to the search filter, so we trim it and check emptiness in
   * order to reset it.
   */
  auto search_key = search_field_text == nullptr ||
                            Util::trim_string_view(search_field_text).empty()
                        ? nullptr
                        : search_field_text;

  ClipboardListManager::update_search_filter_callback(search_key);
}

/**
 * Add or remove a menu field to quit the application depending on the
 * background service setting.
 */
static void update_service_section_menu(GSettings *self, gchar *key,
                                        gpointer user_data) {
  auto app_window = CBWAITA_APP_WINDOW(user_data);

  g_menu_remove_all(app_window->service_section);

  const auto service_mode_enabled = g_settings_get_boolean(self, key);

  gtk_window_set_hide_on_close(GTK_WINDOW(app_window), service_mode_enabled);

  if (service_mode_enabled) {
    g_menu_append(app_window->service_section, "Quit background service",
                  "app.quit");
  }

  auto text_label = service_mode_enabled ? "Background service enabled"
                                         : "Background service disabled";

  gtk_label_set_text(app_window->footer_bg_service, text_label);
  gtk_label_set_text(app_window->footer_bg_service_mobile, text_label);
}

/**
 * This callback is used to track the window closed state and manage the window
 * ownership. See the documentation in "CbwaitaApp::close_window_request".
 */
static auto on_close_request_callback(GtkWindow *, gpointer) -> gboolean {
  g_debug("Close request callback.");

  CbwaitaApp::close_window_request();

  // Invoke the default handler.
  return 0;
}

static void overlay_remove_toast(AppWindow *window) {
  gtk_overlay_remove_overlay(GTK_OVERLAY(window->content_overlay),
                             GTK_WIDGET(window->wrapping_toast));

  window->wrapping_toast = nullptr;
}

static void app_window_dispose(GObject *object) {
  auto self = CBWAITA_APP_WINDOW(object);

  if (self->settings != nullptr) {
    save_window_state(self);
  }

  if (self->wrapping_toast != nullptr) {
    overlay_remove_toast(self);
  }

  g_clear_object(&self->settings);
  g_clear_object(&self->service_section);
  g_clear_object(&self->preferences_dialog);
  g_clear_object(&self->wrapping_toast);

  G_OBJECT_CLASS(app_window_parent_class)->dispose(object);

  g_debug("Application window disposed.");
}

static void app_window_class_init(AppWindowClass *klass) {
  auto object_class = G_OBJECT_CLASS(klass);
  auto widget_class = GTK_WIDGET_CLASS(klass);

  object_class->dispose = app_window_dispose;

  gtk_widget_class_set_template_from_resource(
      widget_class, "/com/github/digitalone1/clipboardwaita/app_window.ui");

  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       window_breakpoint);

  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       clipboard_watcher_switch);
  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       select_all_items_button);
  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       remove_selected_items_button);

  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       search_mode_toggle_button);
  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       selection_mode_toggle_button);
  gtk_widget_class_bind_template_child(widget_class, AppWindow, primary_menu);

  gtk_widget_class_bind_template_child(widget_class, AppWindow, top_search_bar);
  gtk_widget_class_bind_template_child(widget_class, AppWindow, search_entry);

  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       content_overlay);
  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       content_adwtoast_overlay);

  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       clipboard_history_listbox);
  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       list_status_page);
  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       search_status_page);

  gtk_widget_class_bind_template_child(widget_class, AppWindow, footer_mobile);

  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       footer_list_size);
  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       footer_list_size_mobile);
  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       footer_bg_service);
  gtk_widget_class_bind_template_child(widget_class, AppWindow,
                                       footer_bg_service_mobile);
}

static void app_window_init(AppWindow *self) {
  gtk_widget_init_template(GTK_WIDGET(self));

  // Settings
  self->settings = g_settings_new("com.github.digitalone1.clipboardwaita");

  // Service menu is built dinamically upon the background service option.
  self->service_section = g_menu_new();

  // Primary menu is a GMenuModel, but we need a GMenu to append a new section.
  g_menu_append_section(G_MENU(self->primary_menu), nullptr,
                        G_MENU_MODEL(self->service_section));

  // NOTE: GSettings changed signal only emits if the key is read at least once
  // while a signal handler is already connected.
  g_signal_connect(self->settings, "changed::background-service-mode",
                   G_CALLBACK(update_service_section_menu), self);

  const auto background_service_enabled =
      g_settings_get_boolean(self->settings, "background-service-mode");

  gtk_window_set_hide_on_close(GTK_WINDOW(self), background_service_enabled);

  if (background_service_enabled) {
    g_menu_append(self->service_section, "Quit background service", "app.quit");
  }

  // Header bar widgets
  g_settings_bind(self->settings, "clipboard-tracking",
                  self->clipboard_watcher_switch, "active",
                  G_SETTINGS_BIND_DEFAULT);

  g_signal_connect(self->remove_selected_items_button, "clicked",
                   G_CALLBACK(on_remove_selected_items_button_clicked), self);

  g_signal_connect(self->select_all_items_button, "clicked",
                   G_CALLBACK(on_select_all_rows_clicked), self);

  g_object_bind_property(self->search_mode_toggle_button, "active",
                         self->top_search_bar, "search-mode-enabled",
                         G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  g_object_bind_property(self->search_mode_toggle_button, "active",
                         self->list_status_page, "visible",
                         G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE |
                             G_BINDING_INVERT_BOOLEAN);

  g_object_bind_property(self->search_mode_toggle_button, "active",
                         self->search_status_page, "visible",
                         G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  g_signal_connect(self->selection_mode_toggle_button, "toggled",
                   G_CALLBACK(switch_clipboard_listbox_selection_mode), self);

  g_signal_connect(self->search_entry, "search-changed",
                   G_CALLBACK(search_entry_text_changed), nullptr);

  // List box
  auto filtered_list_model =
      G_LIST_MODEL(ClipboardListManager::get_filtered_list_model());

  gtk_list_box_bind_model(
      self->clipboard_history_listbox, filtered_list_model,
      reinterpret_cast<GtkListBoxCreateWidgetFunc>(create_list_item_row),
      nullptr, nullptr);

  // Footer widgets
  auto text_label = background_service_enabled ? "Background service enabled"
                                               : "Background service disabled";

  gtk_label_set_text(self->footer_bg_service, text_label);
  gtk_label_set_text(self->footer_bg_service_mobile, text_label);

  /**
   * Preferences dialog.
   * Normally the dialog is destroyed when closed, but we want to reuse it
   * without having to recreate the object every time the user opens it.
   * To do this, we have to increase the reference count, but we have to use
   * g_object_ref_sink because with the usual g_object_ref the dialog still
   * has it's internal floating reference.
   */
  self->preferences_dialog = preferences_dialog_new();

  g_object_ref_sink(self->preferences_dialog);

  // Initialize window size/maximized state.
  initialize_window_state(self);

  // Close request signal to track the closed state of the window.
  g_signal_connect(self, "close-request", G_CALLBACK(on_close_request_callback),
                   nullptr);

  auto display = gdk_display_get_default();

  // Apply custom CSS for wrapping toasts.
  if (display != nullptr) {
    g_autoptr(GtkCssProvider) provider = gtk_css_provider_new();

    gtk_css_provider_load_from_string(provider, custom_css);

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(), GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }
}

auto app_window_new(GApplication *app) -> AppWindow * {
  return CBWAITA_APP_WINDOW(
      g_object_new(CBWAITA_TYPE_APP_WINDOW, "application", app, nullptr));
}

auto app_window_get_breakpoint(AppWindow *window) -> AdwBreakpoint * {
  return window->window_breakpoint;
}

/**
 * Since Gnome devs forget to implement a property to retrieve the applied
 * state of an AdwBreakpoint, we have to make the following hack:
 */
auto app_window_breakpoint_applied(AppWindow *window) -> gboolean {
  return gtk_action_bar_get_revealed(window->footer_mobile);
}

auto app_window_get_selection_mode_button(AppWindow *window)
    -> GtkToggleButton * {
  return window->selection_mode_toggle_button;
}

void app_window_open_search_bar(AppWindow *window) {
  gtk_toggle_button_set_active(window->search_mode_toggle_button, 1);
}

void app_window_set_selection_mode(AppWindow *window) {
  gtk_toggle_button_set_active(window->selection_mode_toggle_button, 1);
}

void app_window_open_preferences_dialog(AppWindow *window) {
  preferences_dialog_restore(window->preferences_dialog);

  adw_dialog_present(ADW_DIALOG(window->preferences_dialog),
                     GTK_WIDGET(window));
}

void app_window_show_wrapping_toast(AppWindow *window,
                                    const std::string &text) {
  // Remove the previous toast.
  if (window->wrapping_toast != nullptr) {
    overlay_remove_toast(window);
  }

  // Remove all AdwToast, if present.
  adw_toast_overlay_dismiss_all(window->content_adwtoast_overlay);

  window->wrapping_toast = wrapping_toast_new();

  // Attach the new one.
  gtk_overlay_add_overlay(GTK_OVERLAY(window->content_overlay),
                          GTK_WIDGET(window->wrapping_toast));

  // Show the new toast.
  wrapping_toast_set_text(window->wrapping_toast, text);
  wrapping_toast_show(window->wrapping_toast);

  g_debug("New toast has been shown.");
}

void app_window_show_adw_toast(AppWindow *window, const std::string &text,
                               const guint timeout,
                               const AdwToastPriority priority) {
  auto adw_toast = adw_toast_new(text.c_str());

  adw_toast_set_timeout(adw_toast, timeout);
  adw_toast_set_priority(adw_toast, priority);

  // The AdwToast ownership is passed to the AdwToastOverlay, so we don't need
  // to free anything.
  adw_toast_overlay_add_toast(window->content_adwtoast_overlay, adw_toast);
}

void app_window_update_list_size_indicator(AppWindow *window,
                                           const guint n_items,
                                           const guint list_model_size) {
  const auto list_size_label_text =
      n_items == 0 ? "Empty list"
                   : Util::to_string(n_items) + " of " +
                         Util::to_string(list_model_size) + " entries";

  gtk_label_set_text(window->footer_list_size, list_size_label_text.c_str());
  gtk_label_set_text(window->footer_list_size_mobile,
                     list_size_label_text.c_str());
}
