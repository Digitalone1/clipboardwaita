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

#include "clipboard_item_holder.hpp"

enum { PROP_0, PROP_PTR, N_PROPERTIES };

static GParamSpec *properties[N_PROPERTIES] = {nullptr};

struct _ItemHolder {
  GObject parent_instance;

  ClipboardModelItem *ptr;
};

G_DEFINE_TYPE(ItemHolder, item_holder, G_TYPE_OBJECT)

static void item_holder_set_property(GObject *object, guint prop_id,
                                     const GValue *value, GParamSpec *pspec) {
  auto self = CBWAITA_ITEM_HOLDER(object);

  switch (prop_id) {
  case PROP_PTR:
    self->ptr = static_cast<ClipboardModelItem *>(g_value_get_pointer(value));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}

static void item_holder_get_property(GObject *object, guint prop_id,
                                     GValue *value, GParamSpec *pspec) {
  auto self = CBWAITA_ITEM_HOLDER(object);

  switch (prop_id) {
  case PROP_PTR:
    g_value_set_pointer(value, self->ptr);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}

static void item_holder_finalize(GObject *object) {
  auto self = CBWAITA_ITEM_HOLDER(object);

  if (self->ptr != nullptr) {
    delete self->ptr;
    self->ptr = nullptr;
  }

  G_OBJECT_CLASS(item_holder_parent_class)->finalize(object);

  g_debug("Clipboard item holder finalized.");
}

static void item_holder_class_init(ItemHolderClass *klass) {
  auto object_class = G_OBJECT_CLASS(klass);

  object_class->set_property = item_holder_set_property;
  object_class->get_property = item_holder_get_property;
  object_class->finalize = item_holder_finalize;

  properties[PROP_PTR] =
      g_param_spec_pointer("ptr", "Pointer", "Owned ClipboardModelItem pointer",
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void item_holder_init(ItemHolder *self) { self->ptr = nullptr; }

auto item_holder_new(const char *text, const size_t text_hash,
                     const uint64_t timestamp) -> ItemHolder * {
  return CBWAITA_ITEM_HOLDER(g_object_new(
      CBWAITA_TYPE_ITEM_HOLDER, "ptr",
      new ClipboardModelItem(text, text_hash, timestamp), nullptr));
}

auto item_holder_get_data(const ItemHolder *item_holder)
    -> ClipboardModelItem * {
  return item_holder->ptr;
}
