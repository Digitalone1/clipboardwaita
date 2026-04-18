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

#include "util.hpp"
#include <glib.h>

namespace Util {

constexpr const char *const whitespace = " \t\n\r\f\v";
constexpr std::string_view whitespace_except_newline = " \t\r\f\v";

/**
 * Zero-copy basic string trimming that modifies the given string shifting the
 * characters if needed.
 */
void trim_string(std::string &str) {
  const auto first = str.find_first_not_of(whitespace);

  if (first == std::string::npos) {
    // All whitespaces.
    str.clear();

    return;
  }

  const auto last = str.find_last_not_of(whitespace);

  str.erase(last + 1U);
  str.erase(0U, first);
}

/**
 * Zero-copy string trimming that returns a string view, but can be used even
 * on basic strings.
 *
 * Since views are non-owning, this util should be avoided on temporaries:
 * auto v = trim_string_view(std::string("  hello  "));
 *
 * Can be used on string literals instead because they have static lifetime:
 * auto v = trim_string_view("   hello   ");
 */
auto trim_string_view(std::string_view sw) -> std::string_view {
  const auto first = sw.find_first_not_of(whitespace);

  if (first == std::string_view::npos) {
    // All whitespaces.
    return std::string_view();
  }

  sw.remove_prefix(first);

  const auto last = sw.find_last_not_of(whitespace);

  sw.remove_suffix(sw.size() - last - 1U);

  return sw;
}

/**
 * Take a string and reduce all middle whitespaces (except new lines) to a
 * single space. Returns a new string not longer than a given max length.
 * This is used to construct new strings for clipboard item main labels.
 */
auto trim_single_space_string(std::string_view sw, const uint max_length)
    -> std::string {
  const auto sw_size = sw.size();

  std::string single_space_str;

  bool space_appended = false;

  // i counter for trim_sw; size counter for single_space_str.
  for (auto i = 0UL, size = 0UL; i < sw_size && size < max_length; i++) {
    auto c = sw[i];

    if (whitespace_except_newline.find(c) == std::string_view::npos) {
      // Append allowed character.
      single_space_str.push_back(c);

      space_appended = false;

      size++;

      continue;
    }

    // If a not-allowed character is encountered, append a single space only if
    // the previous one is an allowed character.
    if (!space_appended) {
      single_space_str.push_back(' ');

      space_appended = true;

      size++;
    }
  }

  return single_space_str;
}

auto gsettings_get_std_string(GSettings *settings, const gchar *key)
    -> std::string {
  g_autofree gchar *string = g_settings_get_string(settings, key);

  return std::string(string);
}

/**
 * g_free utils that work for both c-strings and constant c-strings.
 */
void g_free_string(char *const str) { g_free(str); };
void g_free_string(const char *const str) { g_free(const_cast<char *>(str)); }

} // namespace Util
