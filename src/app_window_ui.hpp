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

#include <adwaita.h>
#include <glib-object.h>
#include <string>

G_BEGIN_DECLS

#define CBWAITA_TYPE_APP_WINDOW (app_window_get_type())

G_DECLARE_FINAL_TYPE(AppWindow, app_window, CBWAITA, APP_WINDOW, AdwWindow)

G_END_DECLS

auto app_window_new(GApplication *app) -> AppWindow *;

auto app_window_get_breakpoint(AppWindow *window) -> AdwBreakpoint *;

auto app_window_breakpoint_applied(AppWindow *window) -> gboolean;

auto app_window_get_selection_mode_button(AppWindow *window)
    -> GtkToggleButton *;

void app_window_open_search_bar(AppWindow *window);

void app_window_set_selection_mode(AppWindow *window);

void app_window_open_preferences_dialog(AppWindow *window);

void app_window_show_wrapping_toast(AppWindow *window, const std::string &text);

void app_window_show_adw_toast(
    AppWindow *window, const std::string &text, const guint timeout = 0U,
    const AdwToastPriority priority = ADW_TOAST_PRIORITY_HIGH);

void app_window_update_list_size_indicator(AppWindow *window,
                                           const guint n_items,
                                           const guint list_model_size);
