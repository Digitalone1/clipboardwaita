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

#include "app_window_ui.hpp"
#include <array>
#include <gio/gio.h>
#include <gtk/gtk.h>

namespace CbwaitaApp {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
/**
 * Command line GOptions.
 * Unlike the GActionEntry array, this one is null-terminated.
 * The double curly brackets are intentional. Outers to initialize std::array
 * and inners to initialize the underlying C array.
 */
constexpr std::array<GOptionEntry, 3> options{
    {{"toggle-window", 0, 0, G_OPTION_ARG_NONE, nullptr, "Toggle window",
      nullptr},
     {"quit", 'q', 0, G_OPTION_ARG_NONE, nullptr, "Quit running instance",
      nullptr},
     {nullptr}}};
#pragma GCC diagnostic pop

auto get_settings() -> GSettings *;
auto get_window() -> AppWindow *;

void close_window_request();

// Callbacks
auto on_handle_local_options(GApplication *self, GVariantDict *options,
                             gpointer) -> gint;
void on_startup_callback(GApplication *self, gpointer);
void on_activate_callback(GApplication *self, gpointer);
void on_shutdown_callback(GApplication, gpointer);

} // namespace CbwaitaApp
