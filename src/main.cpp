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

    g_autoptr(AdwApplication) app =
        adw_application_new(CbwaitaApp::app_id, G_APPLICATION_DEFAULT_FLAGS);

    if (app == nullptr) {
      return EXIT_FAILURE;
    }

    // Register command line options.
    g_application_add_main_option_entries(G_APPLICATION(app),
                                          CbwaitaApp::options.data());

    g_signal_connect(app, "startup",
                     G_CALLBACK(CbwaitaApp::on_startup_callback), nullptr);
    g_signal_connect(app, "activate",
                     G_CALLBACK(CbwaitaApp::on_activate_callback), nullptr);
    g_signal_connect(app, "handle-local-options",
                     G_CALLBACK(CbwaitaApp::on_handle_local_options), nullptr);
    g_signal_connect(app, "shutdown",
                     G_CALLBACK(CbwaitaApp::on_shutdown_callback), nullptr);

    return g_application_run(G_APPLICATION(app), argc, argv);
  } catch (const std::exception &e) {
    g_critical("%s", e.what());

    return EXIT_FAILURE;
  }
}
