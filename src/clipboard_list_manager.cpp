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

#include "clipboard_list_manager.hpp"
#include "app_window_ui.hpp"
#include "cbwaita_app.hpp"
#include "clipboard_item_holder.hpp"
#include "clipboard_item_row.hpp"
#include "util.hpp"
#include <chrono>
#include <map>

namespace {

// Hash function of strings using string views.
constexpr std::hash<std::string_view> str_hash;

/**
 * The following map is used to quickly discover if a string is contained into
 * the list model using its hash.
 * Instead of looping the whole list, we calculate the hash and ask the map
 * to be sure that a string is contained into the list model.
 * For every hash, various timastamps can be associated.
 * The map always refers to the modified hash (which contains the original
 * hash if the item is unchanged, or the new hash if the item is modified).
 * The timestamps instead always refers to the original timestamp, not the
 * modified one.
 * When an item is modified, the timestamp should be removed from the old
 * hash and associated to the new hash in the map.
 */
std::map<size_t, std::vector<uint64_t>> hash_map;

// List model data
GListStore *list_model = g_list_store_new(CBWAITA_TYPE_ITEM_HOLDER);

GtkCustomFilter *search_filter =
    gtk_custom_filter_new(nullptr, nullptr, nullptr);

/**
 * Filter list model to select only the searched items.
 */
GtkFilterListModel *filtered_list_model = gtk_filter_list_model_new(
    G_LIST_MODEL(list_model), GTK_FILTER(search_filter));

/**
 * Folded tokens of the search key (to be checked against the folded tokens of
 * every items).
 */
gchar **search_folded_tokens = nullptr;

/*
void print_hash_map(
    const std::map<size_t, std::vector<unix_timestamp>> &hash_map) {
  for (auto mit = hash_map.cbegin(); mit != hash_map.cend(); mit++) {
    g_debug("Hash %lu", mit->first);
    for (auto vit = mit->second.cbegin(); vit != mit->second.cend(); vit++) {
      g_debug("   Timestamp %li",
              duration_cast<seconds>((*vit).time_since_epoch()).count());
    }
  }
}
*/

auto hash_map_contains(const size_t hash) -> bool {
  try {
    return !hash_map.at(hash).empty();
  } catch (...) {
    return false;
  }
}

void hash_map_emplace_back_ts(const size_t hash, const uint64_t timestamp) {
  try {
    auto &ts_vector = hash_map.at(hash);

    ts_vector.emplace_back(timestamp);

    g_debug("Emplace back a new timestamp in vector.");
  } catch (...) {
    hash_map.emplace(hash, std::vector<uint64_t>{timestamp});
  }
}

auto hash_map_erase_ts(const size_t hash, const uint64_t timestamp) -> bool {
  try {
    auto &vector_ts = hash_map.at(hash);

    const auto erased_count = std::erase(vector_ts, timestamp);

    const auto erased = erased_count > 0;

    if (erased) {
      g_debug("Removed timestamp %s from the hash map.",
              Util::to_string(timestamp).c_str());
    } else {
      g_warning("Trying to remove a non-existing hash from the hash map.");
    }

    if (vector_ts.empty()) {
      // If there are no timestamps anymore, erase the hash key.
      hash_map.erase(hash);

      g_debug("Removed hash %s from the hash map.",
              Util::to_string(hash).c_str());
    }

    return erased;
  } catch (...) {
    return false;
  }
}

auto hash_map_get_timestamps(const size_t hash) -> std::vector<uint64_t> {
  try {
    return hash_map.at(hash);
  } catch (...) {
    return std::vector<uint64_t>();
  }
}

/**
 * Callback to compare two items. It's used by "g_list_store_insert_sorted" to
 * keep the items sorted.
 * This returns -1 if a<b, 1 if a>b or 0 if a=b.
 * The items are first sorted by the pin flag and, then, by the original
 * timestamp.
 */
auto compare_clipboard_items(gconstpointer a, gconstpointer b, gpointer)
    -> gint {
  auto item_holder_a = static_cast<const ItemHolder *>(a);
  auto item_holder_b = static_cast<const ItemHolder *>(b);

  auto model_item_a = item_holder_get_data(item_holder_a);
  auto model_item_b = item_holder_get_data(item_holder_b);

  // Pinned items should always be shown on top of unpinned ones.
  if (model_item_a->pinned != model_item_b->pinned) {
    return model_item_a->pinned ? -1 : 1;
  }

  /**
   * The following may seem not intuitive, but we have to sort the items from
   * the most recent to the oldest one, so we have to return -1 when the first
   * timestamp is bigger than the second one.
   */
  if (model_item_a->timestamp > model_item_b->timestamp) {
    return -1;
  } else if (model_item_a->timestamp < model_item_b->timestamp) {
    return 1;
  }

  return 0;
}

/**
 * Determines the equality of two items by their timestamps. Note that two
 * items can have equal contents, but they can still differ by the timestamps.
 * This callback is not used, but may be useful in the future in case we need
 * to use "g_list_store_find_with_equal_func".
 */
/*
auto equal_clipboard_items(gconstpointer a, gconstpointer b) -> gboolean {
  auto item_holder_a = static_cast<const ItemHolder *>(a);
  auto item_holder_b = static_cast<const ItemHolder *>(b);

  auto model_item_a = item_holder_get_data(item_holder_a);
  auto model_item_b = item_holder_get_data(item_holder_b);

  return model_item_a->timestamp == cmodel_item_b->timestamp ? 1 : 0;
}
*/

/**
 * When the list model changes, update the list size indicator in the main
 * window.
 */
void on_list_models_items_changed(GListModel *self, guint, guint, guint,
                                  gpointer) {
  const auto n_items = g_list_model_get_n_items(G_LIST_MODEL(self));

  const auto list_model_size =
      g_settings_get_uint(CbwaitaApp::get_settings(), "clipboard-history-size");

  app_window_update_list_size_indicator(CbwaitaApp::get_window(), n_items,
                                        list_model_size);
}

/**
 * When the max size changes in GSettings, invoke a new callback in the main
 * loop to update the size on the list model.
 */
void on_list_max_size_changed(GSettings *self, gchar *key, gpointer) {
  const auto new_list_size = g_settings_get_uint(self, key);

  g_idle_add_once(ClipboardListManager::set_new_list_model_size,
                  GUINT_TO_POINTER(new_list_size));
}

/**
 * This is an helper for "g_list_store_splice" when we want to remove
 * continuous items after a predefined position in the list.
 */
void shrink_list_from_position(const guint position, const guint n_removals,
                               const guint n_items) {
  g_debug("Remove %s items to make space after position %s.",
          Util::to_string(n_removals).c_str(),
          Util::to_string(position).c_str());

  auto list = G_LIST_MODEL(list_model);

  // Remove hashes and timestamps from the hash map.
  for (auto i = position; i < n_items; i++) {
    g_autoptr(ItemHolder) item_holder =
        CBWAITA_ITEM_HOLDER(g_list_model_get_object(list, i));

    auto model_item = item_holder_get_data(item_holder);

    hash_map_erase_ts(model_item->hash_modified, model_item->timestamp);
  }

  // Remove items from the list model.
  g_list_store_splice(list_model, position, n_removals, nullptr, 0);
}

/**
 * Unfortunately GIO List Store does not provide a method to select the item
 * position by a specific value, but we can manage do to it using the binary
 * search since the List Store we use is sorted and the items are uniquely
 * indexed by timestamps.
 *
 * The complexity of this operation is O((log n)^2), which is not ideal, but
 * still better than the O(n) if we loop through all items.
 *
 * The following algorithm may seem counterintuitive because the list model is
 * sorted in reverse order (from the most recent to oldest timestamp), so it's
 * different than the usual binary search.
 *
 * If the n_items argument is not provided by the caller, it's calculated
 * internally before making the search.
 * The method returns a pair with the position and the pointer to the found
 * item. If the item is not found, the position reference is set to n_items.
 */
auto list_store_get_item_position_by_timestamp(const uint64_t timestamp,
                                               const bool pinned,
                                               const int n_items = -1)
    -> std::pair<guint, ClipboardModelItem *> {
  // We need to cast our GIO List Store to a GIO List Model in order to use
  // g_list_model functions.
  auto list = G_LIST_MODEL(list_model);

  // Set num_items in case n_items argument is not set,
  const auto num_items =
      n_items < 0 ? static_cast<int>(g_list_model_get_n_items(list)) : n_items;

  /**
   * Even if the model uses positions in guint type, we need left and right
   * variables as signed int since right can assume -1 to end the while loop.
   * If the timestamp is not found, the object pointer is NULL and position is
   * equal to num_items
   */
  auto left = 0;
  auto right = num_items - 1;

  while (left <= right) {
    const auto mid = left + ((right - left) / 2);

    g_autoptr(ItemHolder) item_holder =
        CBWAITA_ITEM_HOLDER(g_list_model_get_object(list, mid));

    auto model_item = item_holder_get_data(item_holder);

    if (model_item->pinned == pinned && model_item->timestamp == timestamp) {
      return {static_cast<guint>(mid), model_item};
    }

    if (model_item->pinned != pinned) {
      if (model_item->pinned) {
        left = mid + 1;
      } else {
        right = mid - 1;
      }
    } else if (model_item->timestamp > timestamp) {
      left = mid + 1;
    } else {
      right = mid - 1;
    }
  }

  return {static_cast<guint>(num_items), nullptr};
}

auto search_filter_callback(GObject *item, gpointer user_data) -> gboolean {
  auto item_holder = CBWAITA_ITEM_HOLDER(item);

  auto model_item = item_holder_get_data(item_holder);

  // Retrieve item and search folded tokens.
  auto search_tokens = static_cast<gchar **>(user_data);

  // If no search tokens, show the item.
  if (!search_tokens || !search_tokens[0]) {
    return 1;
  }

  auto item_tokens = model_item->folded_tokens;

  // If no item tokens, do not show the item.
  if (!item_tokens || !item_tokens[0]) {
    return 0;
  }

  // In order to show the item, we have to apply the "AND" search logic.
  // Every search token must match at least one item token.
  for (auto i = 0; search_tokens[i] != nullptr; i++) {
    bool found_match = false;

    for (auto j = 0; item_tokens[j] != nullptr; j++) {
      // Check if search_tokens[i] is a prefix of item_tokens[j].
      if (g_str_has_prefix(item_tokens[j], search_tokens[i])) {
        found_match = true;

        break;
      }
    }

    if (!found_match) {
      // One search token didn't match anything.
      return 0;
    }
  }

  // All search tokens were found.
  return 1;
}

/**
 * This method deletes the item using the pinned state and the timestamp (which
 * works as an index for the GIO List Model) since items are sorted by pin flag
 * and timestamps values.
 * It should be slightly efficient than using
 * "g_list_store_find_with_equal_func" to retrieve the position because it
 * relies on "list_store_get_item_position_by_timestamp" which uses the binary
 * search (in reverse order) to select the item position.
 *
 * If the n_items argument is not provided by the caller, it's calculated
 * internally before making the search.
 * This function returns true if the item has been deleted or false if the item
 * has not been found.
 */
auto list_store_remove_item_by_timestamp(const uint64_t timestamp,
                                         const bool pinned,
                                         const int n_items = -1) -> bool {
  const auto num_items =
      n_items < 0
          ? static_cast<int>(g_list_model_get_n_items(G_LIST_MODEL(list_model)))
          : n_items;

  const auto search_result =
      list_store_get_item_position_by_timestamp(timestamp, pinned, num_items);

  if (search_result.first == static_cast<guint>(num_items)) {
    g_debug("Trying to remove a non-existing item from the list model.");

    return false;
  }

  // Remove the item from the list model using its position.
  g_list_store_remove(list_model, search_result.first);

  g_debug("Removed item from the list model with timestamp %s.",
          Util::to_string(timestamp).c_str());

  return true;
}

/**
 * This method should always be used to set the folded_tokens index.
 * Passed search_key is not owned.
 */
void update_search_index(const char *search_key) {
  auto old_folded_tokens = search_folded_tokens;

  // TODO: Investigate if we should set something more spedific for the
  // 2nd and the 3th parameters.
  search_folded_tokens =
      search_key != nullptr
          ? g_str_tokenize_and_fold(search_key, nullptr, nullptr)
          : nullptr;

  // Free previous index
  g_strfreev(old_folded_tokens);
}

} // namespace

namespace ClipboardListManager {

auto get_filtered_list_model() -> GtkFilterListModel * {
  return filtered_list_model;
}

void connect_model_items_changed_signal() {
  g_signal_connect(list_model, "items-changed",
                   G_CALLBACK(on_list_models_items_changed), nullptr);
}

void connect_list_max_size_changed_signal(GSettings *settings) {
  g_signal_connect(settings, "changed::clipboard-history-size",
                   G_CALLBACK(on_list_max_size_changed), nullptr);
}

/**
 * This callback method checks the new size for the clipboard history set by
 * the user.
 * If the new size is lower than the current one, last items are deleted.
 * It should be invoked with g_idle_add_once.
 */
void set_new_list_model_size(gpointer user_data) {
  const auto new_list_size = GPOINTER_TO_UINT(user_data);

  g_debug("Set new size %s for the list model.",
          Util::to_string(new_list_size).data());

  const auto n_items = g_list_model_get_n_items(G_LIST_MODEL(list_model));

  if (new_list_size < n_items) {
    const auto n_removals = n_items - new_list_size;

    g_debug("Remove last %s items of the list model.",
            Util::to_string(n_removals).c_str());

    // Use splice to remove the last (n_items - new_list_size)
    shrink_list_from_position(new_list_size, n_removals, n_items);

    // Here we can return because the footer list size in the UI
    // will be updated by the callback on items-changed signal
    return;
  }

  // Here the model item is not changed, but we update the current number of
  // items in the footer indicator.
  app_window_update_list_size_indicator(CbwaitaApp::get_window(), n_items,
                                        new_list_size);
}

/**
 * Callback in which we receive the new text data retrieved by the clipboard.
 * We take ownership of the text.
 * This function should be called by g_idle_add_once and it assumes empty text
 * is already discarded by the caller.
 *
 */
void add_new_text_callback(gpointer user_data) {
  const auto text = static_cast<const char *>(user_data);

  const auto text_hash = str_hash(text);

  auto list = G_LIST_MODEL(list_model);

  /**
   * Automatically discard if the text is the same as the first entry.
   * This can happen when the first entry has been modified and the modified
   * text has been copied to the clipboard, so it passes the last hash check in
   * the watcher.
   */
  if (g_list_model_get_n_items(list) > 0) {
    g_autoptr(ItemHolder) item_holder =
        CBWAITA_ITEM_HOLDER(g_list_model_get_object(list, 0U));

    auto model_item = item_holder_get_data(item_holder);

    if (model_item->hash_modified == text_hash) {
      Util::g_free_string(text);

      g_debug("Automatically discard the duplicated first item.");

      return;
    }
  }

  auto settings = CbwaitaApp::get_settings();

  const auto duplicate_option_enum =
      g_settings_get_enum(settings, "new-duplicate-entry");

  switch (duplicate_option_enum) {
  case 1: {
    // Do not insert: discard the duplicate entry.
    if (hash_map_contains(text_hash)) {
      Util::g_free_string(text);

      g_debug("Detected a duplicate clipboard item. The item is discarded.");

      return;
    }

    break;
  }

  case 2: {
    /**
     * Remove old duplicates:
     * Delete existing duplicates from the list model, but only if they are
     * unpinned. If there are existing pinned items, the item is discarded.
     */
    if (hash_map_contains(text_hash)) {
      // Copy the vector of timestamps because we may to erase it from the map.
      const auto vector_ts = hash_map_get_timestamps(text_hash);

      bool pinned_item_found = false;
      auto n_items = g_list_model_get_n_items(list);

      for (auto it = vector_ts.cbegin(); it != vector_ts.cend(); it++) {
        // Search for pinned item.
        const auto result =
            list_store_get_item_position_by_timestamp(*it, true, n_items);

        if (result.first != n_items) {
          // Found a pinned item. Skip the removal.
          pinned_item_found = true;

          continue;
        }

        // If the pinned item is not found, we should remove the unpinned
        // variant.
        const auto removed =
            list_store_remove_item_by_timestamp(*it, false, n_items);

        if (removed) {
          n_items--;
        }

        hash_map_erase_ts(text_hash, *it);
      }

      if (pinned_item_found) {
        Util::g_free_string(text);

        g_debug("The new clipboard item has been discarded because there's a "
                "duplicated pinned one.");

        return;
      }
    }

    break;
  }

  default:
    // case 0:
    // Insert: allow duplicate entries.
    break;
  }

  const auto list_max_size =
      g_settings_get_uint(settings, "clipboard-history-size");

  const auto n_items = g_list_model_get_n_items(list);

  if (n_items >= list_max_size) {
    const auto position = list_max_size - 1;
    const auto n_removals = n_items - position;

    // Make space for the new item deleting the last ones.
    shrink_list_from_position(position, n_removals, n_items);
  }

  const uint64_t timestamp =
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();

  hash_map_emplace_back_ts(text_hash, timestamp);

  /**
   * Here we have a reference on the item holder, but we have to drop it with
   * g_autoptr because the list model automatically put another reference on it.
   * The item row also put another reference, but manually.
   *
   * In other words, we pass the ownership of the item to the list model.
   * Also the ownership of the clipboard text content is moved to the item
   * holder.
   */
  g_autoptr(ItemHolder) item_holder =
      item_holder_new(text, text_hash, timestamp);

  g_list_store_insert_sorted(list_model, item_holder, compare_clipboard_items,
                             nullptr);

  g_debug("New clipboard item inserted into the list model.");

  return;
}

/**
 * Here we handle the request to pin or unpin a single item.
 * Unfortunately the GIO List Store does not arrange the items automatically
 * when they change internally.
 * We could use the "g_list_store_sort" function, but it's not very efficient
 * and it emits the "items-changed" signal many times which is not ideal.
 *
 * A more efficient workaround is to remove the selected item and insert its
 * copy with the pinned flag inverted.
 * The callback receives the targeted item holder (not owned) and its relevant
 * data is copied. Then the holder is deleted and a new item is created with the
 * copied data plus the inverted pinned flag.
 *
 * This function should be called by g_idle_add_once.
 */
void swap_pinned_item(gpointer user_data) {
  auto item_holder = CBWAITA_ITEM_HOLDER(user_data);

  auto model_item = item_holder_get_data(item_holder);

  // Text and text modified strings should be copied/duplicated with a new
  // allocation.
  const auto text = g_strdup(model_item->text);
  const auto text_modified = g_strdup(model_item->text_modified);
  const auto hash = model_item->hash;
  const auto hash_modified = model_item->hash_modified;
  const auto timestamp = model_item->timestamp;
  const auto timestamp_modified = model_item->timestamp_modified;
  const auto pinned = model_item->pinned;

  // We do not need to erase the timestamp from the hash map since we have to
  // insert a new item with the same hash_modified and (original) timestamp.
  list_store_remove_item_by_timestamp(timestamp, pinned);

  /**
   * Here we have a reference on the duplicated item holder, but we have to
   * drop it with g_autoptr because the list model automatically put another
   * reference on it. The item row also put another reference, but manually.
   *
   * In other words, we pass the ownership of the item to the list model.
   * Also the ownership of text and text_modified strings is moved to the item
   * holder.
   */
  g_autoptr(ItemHolder) item_holder_duplicated =
      item_holder_duplicate(text, hash, text_modified, hash_modified, timestamp,
                            timestamp_modified, !pinned);

  g_list_store_insert_sorted(list_model, item_holder_duplicated,
                             compare_clipboard_items, nullptr);

  g_debug("Duplicated item inserted with timestamp %s into the list model.",
          Util::to_string(timestamp).c_str());

  return;
}

/**
 * Update the filter list model with the GtkCustomFilterFunc using the
 * search_folded_tokens.
 * Passed search_key is not owned.
 */
void update_search_filter_callback(const char *search_key) {
  update_search_index(search_key);

  auto match_func =
      search_key != nullptr
          ? reinterpret_cast<GtkCustomFilterFunc>(search_filter_callback)
          : nullptr;

  gtk_custom_filter_set_filter_func(search_filter, match_func,
                                    search_folded_tokens, nullptr);
}

/**
 * Callback to remove the selected rows from the list.
 * The argument takes a GList containing the selected GtkListBoxRow.
 * The function has the ownership of the GList, but does not own the data
 * inside it. The GList should be unreferenced.
 * This callback should be invoked with g_idle_add_once.
 */
void list_model_remove_selected_items(gpointer user_data) {
  auto selected_rows = static_cast<GList *>(user_data);

  std::vector<std::pair<uint64_t, bool>> vector_pairs;

  for (auto r = selected_rows; r != nullptr; r = r->next) {
    auto row = GTK_LIST_BOX_ROW(r->data);

    auto item_row = CBWAITA_ITEM_ROW(gtk_list_box_row_get_child(row));

    if (item_row == nullptr) {
      continue;
    }

    auto item_holder = item_row_get_holder(item_row);

    if (item_holder == nullptr) {
      continue;
    }

    auto model_item = item_holder_get_data(item_holder);

    hash_map_erase_ts(model_item->hash_modified, model_item->timestamp);

    vector_pairs.emplace_back(
        std::make_pair(model_item->timestamp, model_item->pinned));
  }

  const auto n_items = g_list_model_get_n_items(G_LIST_MODEL(list_model));

  const auto items_to_remove = vector_pairs.size();
  const auto clear_list = items_to_remove >= n_items;

  if (clear_list) {
    // If the selected rows are all the items in the list model, just call
    // g_list_store_remove_all:
    g_list_store_remove_all(list_model);

    g_debug("List model cleared.");
  } else {
    auto num_items = n_items;

    for (auto it = vector_pairs.cbegin(); it != vector_pairs.cend(); it++) {
      const auto removed =
          list_store_remove_item_by_timestamp(it->first, it->second, num_items);

      if (removed) {
        num_items--;
      }
    }
  }

  auto app_window = CbwaitaApp::get_window();

  // We show an adw toast only if more than one items have been removed.
  if (app_window != nullptr && items_to_remove > 1U) {
    const auto toast_text = clear_list ? "Clipboard history cleared"
                                       : Util::to_string(items_to_remove) +
                                             " items have been removed";

    app_window_show_adw_toast(app_window, toast_text, 2U);
  }

  g_list_free(selected_rows);
}

/**
 * Update the model item assigning new_text to text_modified.
 * The hash map is updated removing the old hash and adding the new one (moving
 * the timestamp accordingly).
 * The new text is owned, but the ownership is passed to the model item (which
 * can also free it if necessary, so we don't have to touch it after the update
 * has been completed).
 */
void update_model_item(ClipboardModelItem *model_item, const char *new_text) {
  // Create the new timestamp (we need it only for the UI).
  model_item->timestamp_modified =
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();

  // Remove the old hash from the hash map.
  hash_map_erase_ts(model_item->hash_modified, model_item->timestamp);

  // Update the model item. New hash and folded tokens are automatically
  // updated.
  model_item->update_text_modified(new_text);

  // Insert the new hash_modified in the hash map.
  hash_map_emplace_back_ts(model_item->hash_modified, model_item->timestamp);
}

} // namespace ClipboardListManager
