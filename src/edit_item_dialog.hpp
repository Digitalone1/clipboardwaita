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

#include "clipboard_item_row.hpp"

G_BEGIN_DECLS

#define CBWAITA_TYPE_EDIT_ITEM_DIALOG (edit_item_dialog_get_type())

G_DECLARE_FINAL_TYPE(EditItemDialog, edit_item_dialog, CBWAITA,
                     EDIT_ITEM_DIALOG, AdwDialog)

G_END_DECLS

auto edit_item_dialog_new(ItemRow *item_row) -> EditItemDialog *;
