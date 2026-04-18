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

#include "util.hpp"
#include <glib-object.h>

/**
 * The instances of this class take ownership of the text and are responsible
 * of freeing it.
 * The text should be NON-NULL. Sending a NULL pointer to the constructor or the
 * text_modified property leads to an undefined behavior.
 */
class ClipboardModelItem {
public:
  ClipboardModelItem(const char *text, const size_t text_hash,
                     const uint64_t timestamp)
      : timestamp(timestamp), timestamp_modified(timestamp), text(text),
        text_modified(text), hash(text_hash), hash_modified(text_hash) {
    // Save folded tokens for search stage.
    update_item_search_index(text);
  }

  ClipboardModelItem() = delete;
  ClipboardModelItem(const ClipboardModelItem &) = delete;
  ClipboardModelItem(ClipboardModelItem &&) = delete;
  auto operator=(const ClipboardModelItem &) -> ClipboardModelItem & = delete;
  auto operator=(ClipboardModelItem &&) -> ClipboardModelItem & = delete;

  ~ClipboardModelItem() {
    g_strfreev(folded_tokens);

    if (text == text_modified) {
      Util::g_free_string(text);
    } else {
      Util::g_free_string(text);
      Util::g_free_string(text_modified);
    }

    g_debug("Clipboard model item destroyed.");
  }

  static constexpr std::hash<std::string_view> str_hash{};

  /**
   * The (original) timestamp is useful to sort the items in the GIO ListStore
   * and ensures we have unique items in the ListStore internal structure.
   * Even if two items can be considered equal having the same text content,
   * they will always differ by the timestamp.
   * The timestamp is also useful, in some situations, when we cannot use the
   * item position to retrieve a specific item. Indeed we can retrieve the
   * reference to an item using the timestamp as search key (with custom search
   * methods not provided by GIO ListStore).
   */
  const uint64_t timestamp;

  /**
   * timestamp_modified is only meaningful to be shown in the UI. If its value
   * is different than the (original) timestamp, it does not mean the item is
   * modified because the current text content could be reverted back to the
   * original one.
   */
  uint64_t timestamp_modified;

  // Not-NULL item text content.
  const char *const text;

  /**
   * Non-NULL item text_modified content.
   * If the (original) hash is equal to the hash_modified, the content of
   * text_modified is meaningless.
   * Do NOT set this directly, use the update_text_modified setter.
   */
  const char *text_modified;

  /**
   * The following hashes determine the state of the item. If (original) hash
   * is different than the hash_modified, the item is in the "modified" state.
   * Do NOT set hash_modified directly. It is automatically updated using the
   * update_text_modified setter
   */
  const size_t hash;
  size_t hash_modified;

  // Tokens for searching stages.
  // Do NOT set this directly, use the update_item_search_index setter.
  gchar **folded_tokens = nullptr;

  /**
   * This method should always be used to set text_modified.
   * "new_text" ownership is passed here and it MUST be non-NULL.
   * Folded tokens are automatically updated.
   */
  void update_text_modified(const char *new_text) {
    const auto new_text_hash = str_hash(new_text);

    if (new_text_hash == hash) {
      if (new_text != text) {
        Util::g_free_string(new_text);
      }

      text_modified = text;
    } else {
      // We have to free text_modified only if its pointer is different than
      // the one of the original text.
      auto old_text_modified =
          (text_modified != text) ? text_modified : nullptr;

      text_modified = new_text;

      Util::g_free_string(old_text_modified);
    }

    hash_modified = new_text_hash;

    g_debug("Item text_modified content updated.");

    // Folded tokens should always be updated, even if text_modified* == text*.
    update_item_search_index(text_modified);
  }

  // This method should always be used to set folded_tokens index.
  void update_item_search_index(const char *text) {
    auto old_folded_tokens = folded_tokens;

    // TODO: Investigate if we should set something more spedific for the
    // 2nd and the 3th parameters.
    folded_tokens = g_str_tokenize_and_fold(text, nullptr, nullptr);

    // Free previous index
    g_strfreev(old_folded_tokens);

    g_debug("Item folded tokens updated.");
  }
};

G_BEGIN_DECLS

#define CBWAITA_TYPE_ITEM_HOLDER (item_holder_get_type())

G_DECLARE_FINAL_TYPE(ItemHolder, item_holder, CBWAITA, ITEM_HOLDER, GObject)

G_END_DECLS

auto item_holder_new(const char *text, const size_t text_hash,
                     const uint64_t timestamp) -> ItemHolder *;

auto item_holder_get_data(const ItemHolder *item_holder)
    -> ClipboardModelItem *;
