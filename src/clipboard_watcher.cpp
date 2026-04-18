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

#include "clipboard_watcher.hpp"
#include "clipboard_list_manager.hpp"

namespace {

constexpr guint poll_ms_interval_ = 1000U;

bool first_iteration = true;

std::size_t last_hash;
constexpr std::hash<std::string_view> str_hash;

constexpr std::string_view empty_str_view;
const auto empty_str_hash = str_hash(empty_str_view);

guint poll_source_id = 0U;

/**
 * Retrieve clipboard text and calculate its hash.
 * Track the text if the content is new and save its hash.
 * This method should be called only by poll_clipboard.
 */
void clipboard_text_ready(GObject *source, GAsyncResult *res, gpointer) {
  g_autoptr(GError) error = nullptr;

  // We take ownership of the text. If it is tracked inside the list model, we
  // should free it when the item will be deleted.
  auto text =
      gdk_clipboard_read_text_finish(GDK_CLIPBOARD(source), res, &error);

  if (error != nullptr) {
    g_warning_once("An error occurred while retrieving the clipboard data: %s",
                   error->message);

    Util::g_free_string(text);

    return;
  }

  // The text is likely NULL when the clipboard is empty at the system startup.
  // In this case we treat the content as an empty string.
  const auto text_hash = text == nullptr ? empty_str_hash : str_hash(text);

  if (first_iteration) {
    // On the first iteration, we do not want to store the content, but only
    // initialize the last hash.
    last_hash = text_hash;

    g_debug("Last hash initialized in ClibpoardWatcher.");

    first_iteration = false;

    return;
  }

  // Compare the content hash with the last hash.
  if (text_hash == last_hash) {
    Util::g_free_string(text);

    return;
  }

  // Save the new last hash.
  last_hash = text_hash;

  // But return if the text is empty.
  if (text == nullptr || std::string_view(text) == empty_str_view) {
    return;
  }

  // g_debug("New clipboard text: %s", text);

  // Save the text in the GTK list box.
  if (ClipboardWatcher::is_running()) {
    g_idle_add_once(ClipboardListManager::add_new_text_callback, text);
  }
}

/**
 * Retrieve clipboard object and request its data.
 * This method should be executed only by GLib timeout system.
 */
gboolean poll_clipboard(gpointer) {
  auto clipboard = ClipboardWatcher::get_gdk_clipboard();

  if (clipboard == nullptr) {
    g_warning_once("Clipboard reference is not available.");

    return ClipboardWatcher::is_running() ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
  }

  /**
   * In case we have to check for clipboard set by our app:
   * if (gdk_clipboard_is_local(clipboard)) return G_SOURCE_CONTINUE;
   */

  /**
   * gdk_clipboard_read_text_async should deserialize to G_TYPE_STRING
   * automatically, so no need of formats check:
   * GdkContentFormats *formats = gdk_clipboard_get_formats(clipboard);
   * if (gdk_content_formats_contain_gtype(formats, G_TYPE_STRING)) {...}
   */

  if (!ClipboardWatcher::is_running()) {
    return G_SOURCE_REMOVE;
  }

  gdk_clipboard_read_text_async(clipboard, nullptr, clipboard_text_ready,
                                nullptr);

  return G_SOURCE_CONTINUE;
}

void on_clipboard_tracking_switched(GSettings *self, gchar *key, gpointer) {
  const auto tracking_clipboard_enabled =
      g_settings_get_boolean(self, key) != 0;

  if (!tracking_clipboard_enabled) {
    ClipboardWatcher::stop_service();
  } else if (!ClipboardWatcher::is_running()) {
    ClipboardWatcher::start_service();
  }
}

} // namespace

namespace ClipboardWatcher {

auto is_running() -> bool { return poll_source_id != 0U; }

/**
 * Start the clipboard polling service.
 * It's an error calling this function when the service is already started
 * (check if it's enabled first).
 */
void start_service() {
  poll_source_id = g_timeout_add_full(G_PRIORITY_DEFAULT, poll_ms_interval_,
                                      poll_clipboard, nullptr, nullptr);

  g_debug("Started ClipboardWatcher service.");
}

/**
 * Stop the clipboard polling service.
 */
void stop_service() {
  if (poll_source_id != 0U) {
    g_source_remove(poll_source_id);

    poll_source_id = 0U;
  }

  last_hash = 0U;

  first_iteration = true;

  g_debug("Stopped ClipboardWatcher service.");
}

auto get_gdk_clipboard() -> GdkClipboard * {
  auto display = gdk_display_get_default();

  if (display == nullptr) {
    g_warning_once("Display reference is not available.");

    return nullptr;
  }

  return gdk_display_get_clipboard(display);
}

void connect_clipboard_tracking_changed_signal(GSettings *settings) {
  g_signal_connect(settings, "changed::clipboard-tracking",
                   G_CALLBACK(on_clipboard_tracking_switched), nullptr);
}

} // namespace ClipboardWatcher
