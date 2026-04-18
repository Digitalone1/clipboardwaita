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

G_BEGIN_DECLS

#define CBWAITA_TYPE_ITEM_ROW (item_row_get_type())

G_DECLARE_FINAL_TYPE(ItemRow, item_row, CBWAITA, ITEM_ROW, GtkBox)

G_END_DECLS

auto item_row_new(GObject *item_holder) -> ItemRow *;

auto item_row_get_holder(ItemRow *item_row) -> ItemHolder *;

void item_row_update_widgets_state(ItemRow *item_row);
