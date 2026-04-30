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
#include <charconv>
#include <gio/gio.h>
#include <string>

// Glib helpers should not be wrapped in a namespace to work.
#define ENABLE_FLAGS(EnumType)                                                 \
  constexpr EnumType operator|(EnumType a, EnumType b) {                       \
    return static_cast<EnumType>(static_cast<int>(a) | static_cast<int>(b));   \
  }

ENABLE_FLAGS(GBindingFlags)
ENABLE_FLAGS(GSettingsBindFlags)
ENABLE_FLAGS(GParamFlags)

namespace Util {

void trim_string(std::string &str);
auto trim_string_view(std::string_view s) -> std::string_view;
auto trim_single_space_string(std::string_view sw, const uint max_length)
    -> std::string;

auto gsettings_get_std_string(GSettings *settings, const gchar *key)
    -> std::string;

void g_free_string(char *const str);
void g_free_string(const char *const str);

/**
 * This template comverts numbers to strings using `std::to_chars`.
 * An additional string parameter could be eventually provided with a
 * default value to return in case the conversion fails.
 */
template <typename T>
auto to_string(const T num, const std::string &def = "0") -> std::string {
  /**
   * Max buffer length:
   * number of base-10 digits that can be represented by the type T without
   * change + number of base-10 digits that are necessary to uniquely represent
   * all distinct values of the type T (meaningful only for real numbers) + room
   * for other characters such as "+-e,.".
   */
  static constexpr auto max = std::numeric_limits<T>::digits10 +
                              std::numeric_limits<T>::max_digits10 + 10;

  // Avoid static storage on the buffer since we do not want to access to the
  // same buffer from different threads, even if it's unlikely to happen.
  std::array<char, max> buffer;

  const auto p_init = buffer.data();
  const auto p_end = p_init + max;

  const auto result = std::to_chars(p_init, p_end, num);

  return result.ec == std::errc() ? std::string(p_init, result.ptr - p_init)
                                  : def;
}

/**
 * This template binds a GSettings enum key to the selected property of a
 * "combo row type widget", such as AdwComboRow and GtkDropDown.
 */
template <typename T>
void g_settings_bind_combo_row_with_mapping(
    GSettings *settings, const gchar *key, T widget,
    const GSettingsBindFlags flags = G_SETTINGS_BIND_DEFAULT) {
  static_assert(std::is_same_v<T, GtkDropDown *> ||
                std::is_same_v<T, AdwComboRow *>);

  struct Data {
    GSettings *settings;

    const gchar *key;
  };

  g_settings_bind_with_mapping(
      settings, key, widget, "selected", flags,
      +[](GValue *value, GVariant *, gpointer user_data) {
        auto data = static_cast<Data *>(user_data);

        g_value_set_uint(value, static_cast<guint>(g_settings_get_enum(
                                    data->settings, data->key)));

        return 1;
      },
      +[](const GValue *value, const GVariantType *, gpointer user_data) {
        auto data = static_cast<Data *>(user_data);

        g_settings_set_enum(data->settings, data->key,
                            static_cast<gint>(g_value_get_uint(value)));

        const auto string = gsettings_get_std_string(data->settings, data->key);

        return g_variant_new_string(string.c_str());
      },
      new Data({.settings = settings, .key = key}),
      +[](gpointer user_data) {
        auto data = static_cast<Data *>(user_data);

        delete data;
      });
}

} // namespace Util
