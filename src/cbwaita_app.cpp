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

#include "cbwaita_app.hpp"
#include "clipboard_list_manager.hpp"
#include "clipboard_watcher.hpp"

namespace {

GSettings *settings = g_settings_new(CbwaitaApp::app_id);
AppWindow *window = nullptr;

bool start_hidden = false;
bool window_closed = true;

/**
 * This is used to get the Gtk Window from the Gtk Application object.
 * If no window is present, it returns null.
 * We support a single window instance, no multiple ones.
 */
/*
auto get_main_gtk_window(GtkApplication *app) -> GtkWindow * {
  auto windows_list = gtk_application_get_windows(app);

  return windows_list != nullptr ? GTK_WINDOW(windows_list->data) : nullptr;
}
*/

/**
 * When the application has no window on the first launch, this method is
 * invoked to create a new window.
 * We support a single window instance, no multiple ones, so this method
 * should be called only one time. Invoking it multiple times is an error.
 * Since we need the AppWindow object pointer to perform various tasks with the
 * UI widgets, we store its address from the new object returned in this method.
 */
void create_new_window(GApplication *app) {
  /**
   * The app owns the window, but we increase the reference count to attach
   * also our ownership in this singleton. We should unref it when the app is
   * quit (or the window is closed when the background service is disabled).
   */
  window = g_object_ref(app_window_new(app));

  g_debug("New window created.");

  ClipboardListManager::connect_model_items_changed_signal();

  ClipboardListManager::connect_list_max_size_changed_signal(settings);

  ClipboardWatcher::connect_clipboard_tracking_changed_signal(settings);

  const auto tracking_clipboard_enabled =
      g_settings_get_boolean(settings, "clipboard-tracking") != 0;

  if (tracking_clipboard_enabled && !ClipboardWatcher::is_running()) {
    ClipboardWatcher::start_service();
  }
}

void open_window(GtkWindow *window_gtk) {
  g_debug("Open the window");

  window_closed = false;

  gtk_window_present(window_gtk);
}

/**
 * Send the request to close the window.
 * Differently from the open_window function, here it's better if we do not
 * set the window closed state and let the close-request signal callback make it
 * because there we can track also when the user sends the request clicking on
 * the close button in the header bar.
 */
void close_window(GtkWindow *window_gtk) {
  g_debug("Close the window.");

  gtk_window_close(window_gtk);
}

void action_clipboard_tracking(GSimpleAction *, GVariant *, gpointer) {
  const auto clipboard_tracking_enabled =
      g_settings_get_boolean(settings, "clipboard-tracking");

  g_settings_set_boolean(settings, "clipboard-tracking",
                         !clipboard_tracking_enabled);
}

void action_background(GSimpleAction *, GVariant *, gpointer) {
  const auto background_service_enabled =
      g_settings_get_boolean(settings, "background-service-mode");

  g_settings_set_boolean(settings, "background-service-mode",
                         !background_service_enabled);
}

void action_search(GSimpleAction *, GVariant *, gpointer) {
  g_debug("Search action.");

  if (window == nullptr) {
    return;
  }

  app_window_open_search_bar(window);
}

void action_selection(GSimpleAction *, GVariant *, gpointer) {
  g_debug("Selection action.");

  if (window == nullptr) {
    return;
  }

  app_window_set_selection_mode(window);
}

void action_preferences(GSimpleAction *, GVariant *, gpointer) {
  g_debug("Preferences action.");

  if (window == nullptr) {
    return;
  }

  app_window_open_preferences_dialog(window);
}

// TODO: Implement help.
void action_help(GSimpleAction *, GVariant *, gpointer) {
  g_debug("Help action.");
}

void action_about(GSimpleAction *, GVariant *, gpointer) {
  g_debug("About action.");

  if (window == nullptr) {
    return;
  }

  static constexpr std::array<const char *const, 2> developers = {
      "Giusy Digitalone1 <kurmikon@yahoo.com>", nullptr};

  // TODO: add version, website, issue-url and translator-credits.
  adw_show_about_dialog(
      GTK_WIDGET(window), "application-name", "Clipboard History",
      "application-icon", CbwaitaApp::app_id, "developer-name",
      "Giusy Digitalone1", "developers", developers.data(), "copyright",
      "© 2026-2027 Digitalone1", "license-type", GTK_LICENSE_GPL_3_0, nullptr);
}

/**
 * This is a special action invoked by the command line option that open or
 * close the window based on the window_closed bool value.
 * In normal mode the app is quit, while in the stardard mode the window is
 * hidden.
 */
void action_toggle_window(GSimpleAction *, GVariant *, gpointer) {
  g_debug("Toggle window action.");

  if (window == nullptr) {
    return;
  }

  if (window_closed) {
    open_window(GTK_WINDOW(window));
  } else {
    close_window(GTK_WINDOW(window));
  }
}

void action_close_window(GSimpleAction *, GVariant *, gpointer) {
  g_debug("Close window action.");

  if (window == nullptr) {
    return;
  }

  close_window(GTK_WINDOW(window));
}

void action_quit(GSimpleAction *, GVariant *, gpointer user_data) {
  g_debug("Quit action.");

  if (window == nullptr) {
    // If the window has not been created, just quit the g_application.
    g_application_quit(G_APPLICATION(user_data));

    return;
  }

  ClipboardWatcher::stop_service();

  /**
   * Clear the list model in order to dispose the GtkListBoxRows before the
   * window is disposed.
   * In some cases we observed that on quit action the GIO list model is
   * disposed after the window object is destroyed and this leaves the
   * GtkListBoxRows still alive (which is an unwanted behavior) because they
   * are owned by the GIO list model.
   */
  ClipboardListManager::clear_list_model_and_hash_map();

  g_object_unref(window);

  window_closed = true;

  // Quit action bypasses the window close logic, so we just destroy the window.
  gtk_window_destroy(GTK_WINDOW(window));
}

// GActions.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
constexpr std::array<GActionEntry, 10> actions{
    {{"clipboard-tracking", action_clipboard_tracking, nullptr, nullptr,
      nullptr},
     {"background", action_background, nullptr, nullptr, nullptr},
     {"search", action_search, nullptr, nullptr, nullptr},
     {"selection", action_selection, nullptr, nullptr, nullptr},
     {"preferences", action_preferences, nullptr, nullptr, nullptr},
     {"help", action_help, nullptr, nullptr, nullptr},
     {"about", action_about, nullptr, nullptr, nullptr},
     {"toggle-window", action_toggle_window, nullptr, nullptr, nullptr},
     {"close-window", action_close_window, nullptr, nullptr, nullptr},
     {"quit", action_quit, nullptr, nullptr, nullptr}}};
#pragma GCC diagnostic pop

// Shortcuts
constexpr std::array<const char *const, 2> clipboard_tracking_accel{
    "<Control>t", nullptr};
constexpr std::array<const char *const, 2> background_service_accel{
    "<Control>b", nullptr};
constexpr std::array<const char *const, 2> search_mode_accel{"<Control>f",
                                                             nullptr};
constexpr std::array<const char *const, 2> selection_mode_accel{"<Control>s",
                                                                nullptr};
constexpr std::array<const char *const, 2> preferences_accel{"<Control>comma",
                                                             nullptr};
constexpr std::array<const char *const, 2> shortcuts_accel{"<Control>question",
                                                           nullptr};
constexpr std::array<const char *const, 2> help_accel{"F1", nullptr};
constexpr std::array<const char *const, 2> close_window_accel{"<Control>w",
                                                              nullptr};
constexpr std::array<const char *const, 2> quit_accel{"<Control>q", nullptr};

} // namespace

namespace CbwaitaApp {

auto get_window() -> AppWindow * { return window; }

auto get_settings() -> GSettings * { return settings; }

/**
 * This callback returns:
 * - a positive value for failure and exiting the process.
 * - 0 for success and stopping the default processing flow.
 * - -1 to continue the default processing flow (startup, activate, etc...).
 */
auto on_handle_local_options(GApplication *self, GVariantDict *options,
                             gpointer) -> gint {
  g_debug("Handle local options callback.");

  g_autoptr(GError) error = nullptr;

  if (!g_application_register(self, nullptr, &error)) {
    g_warning("An error occurred while registering the application: %s",
              error != nullptr ? error->message : "Unknown error");

    return 1;
  }

  if (g_variant_dict_contains(options, "start-hidden")) {
    g_autoptr(GError) error = nullptr;

    if (g_application_get_is_remote(self)) {
      g_message("The application is already started.");

      return 1;
    }

    // The start-hidden option only works if the background service is enabled.
    const auto background_service_enabled =
        g_settings_get_boolean(settings, "background-service-mode");

    if (background_service_enabled) {
      g_debug("Start the application with the window closed.");

      start_hidden = true;
    } else {
      g_message("To start with the window closed, enable the background "
                "service mode from the preferences.");
    }

    return -1;
  }

  if (g_variant_dict_contains(options, "toggle-window")) {
    g_autoptr(GError) error = nullptr;

    // We want to toggle the window only if the app is already started.
    if (!g_application_get_is_remote(self)) {
      g_message("To toggle the window, start the application earlier.");

      return 1;
    }

    g_action_group_activate_action(G_ACTION_GROUP(self), "toggle-window",
                                   nullptr);

    return 0;
  }

  if (g_variant_dict_contains(options, "quit")) {
    g_autoptr(GError) error = nullptr;

    // If the app is not started, we have nothing to quit.
    if (!g_application_get_is_remote(self)) {
      g_info("The application is already quit.");

      return 1;
    }

    g_action_group_activate_action(G_ACTION_GROUP(self), "quit", nullptr);

    return 0;
  }

  // Continue normal startup.
  return -1;
}

void on_startup_callback(GApplication *self, gpointer) {
  g_debug("Startup callback.");

  // Register Actions
  g_action_map_add_action_entries(G_ACTION_MAP(self), actions.data(),
                                  G_N_ELEMENTS(actions), self);

  auto self_gtk = GTK_APPLICATION(self);

  /**
   * Bind shortcuts for actions.
   * "toggle-window" action won't have a shortcut because we want it to be
   * activated only by the command line related option.
   */
  gtk_application_set_accels_for_action(self_gtk, "app.clipboard-tracking",
                                        clipboard_tracking_accel.data());
  gtk_application_set_accels_for_action(self_gtk, "app.background",
                                        background_service_accel.data());
  gtk_application_set_accels_for_action(self_gtk, "app.search",
                                        search_mode_accel.data());
  gtk_application_set_accels_for_action(self_gtk, "app.selection",
                                        selection_mode_accel.data());
  gtk_application_set_accels_for_action(self_gtk, "app.preferences",
                                        preferences_accel.data());
  gtk_application_set_accels_for_action(self_gtk, "app.shortcuts",
                                        shortcuts_accel.data());
  gtk_application_set_accels_for_action(self_gtk, "app.help",
                                        help_accel.data());
  gtk_application_set_accels_for_action(self_gtk, "app.close-window",
                                        close_window_accel.data());
  gtk_application_set_accels_for_action(self_gtk, "app.quit",
                                        quit_accel.data());
}

void on_activate_callback(GApplication *self, gpointer) {
  g_debug("Activate callback.");

  if (window == nullptr) {
    create_new_window(self);
  }

  if (start_hidden) {
    // Do not open the window when the background service is enabled and
    // the start-hidden command line option has been specified.
    start_hidden = false;
  } else {
    open_window(GTK_WINDOW(window));
  }
}

void on_shutdown_callback(GApplication, gpointer) {
  g_debug("Shutdown callback.");
}

/**
 * This is invoked by the callback of the window close-request signal.
 * Here we track the window closed state and, if the background service is
 * disabled, we decrease the reference on the window object because the app is
 * going to be quit.
 */
void close_window_request() {
  window_closed = true;

  const auto background_service_enabled =
      g_settings_get_boolean(settings, "background-service-mode") != 0;

  if (!background_service_enabled) {
    ClipboardWatcher::stop_service();

    ClipboardListManager::clear_list_model_and_hash_map();

    g_object_unref(window);
  }
}

} // namespace CbwaitaApp
