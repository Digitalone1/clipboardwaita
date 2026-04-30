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

#include "cbwaita_app.hpp"

int main(int argc, char *argv[]) {
  try {
    g_debug("Main function.");

    g_autoptr(AdwApplication) adw_app =
        adw_application_new(CbwaitaApp::app_id, G_APPLICATION_DEFAULT_FLAGS);

    if (adw_app == nullptr) {
      return EXIT_FAILURE;
    }

    auto g_app = G_APPLICATION(adw_app);

    // Register command line options.
    g_application_add_main_option_entries(g_app, CbwaitaApp::options.data());
    g_application_set_option_context_summary(
        g_app, "A clipboard manager with history features.");
    g_application_set_option_context_description(
        g_app, "This application collects clipboard data and may contain "
               "sensitive data.");

    g_signal_connect(adw_app, "startup",
                     G_CALLBACK(CbwaitaApp::on_startup_callback), nullptr);
    g_signal_connect(adw_app, "activate",
                     G_CALLBACK(CbwaitaApp::on_activate_callback), nullptr);
    g_signal_connect(adw_app, "handle-local-options",
                     G_CALLBACK(CbwaitaApp::on_handle_local_options), nullptr);
    g_signal_connect(adw_app, "shutdown",
                     G_CALLBACK(CbwaitaApp::on_shutdown_callback), nullptr);

    return g_application_run(g_app, argc, argv);
  } catch (const std::exception &e) {
    g_critical("%s", e.what());

    return EXIT_FAILURE;
  }
}
