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

#include "clipboard_item_holder.hpp"

namespace ClipboardListManager {

void connect_model_items_changed_signal();
void connect_list_max_size_changed_signal(GSettings *settings);

auto get_filtered_list_model() -> GtkFilterListModel *;

void add_new_text_callback(gpointer user_data);

void swap_pinned_item(gpointer user_data);

void set_new_list_model_size(gpointer user_data);

void update_search_filter_callback(const char *const search_key);

void list_model_remove_selected_items(gpointer user_data);

void clear_list_model_and_hash_map();

void update_model_item(ClipboardModelItem *model_item,
                       const char *const new_text);

} // namespace ClipboardListManager
