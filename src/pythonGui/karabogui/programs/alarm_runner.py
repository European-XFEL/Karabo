# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import argparse
import sys

from qtpy.QtWidgets import QApplication

from karabogui.panels.alarmpanel import AlarmPanel
from karabogui.programs.base import create_gui_app, init_gui
from karabogui.singletons.api import get_network, get_panel_wrangler
from karabogui.util import process_qt_events


def check_desktop_width():
    """Return an appropriate width with respect to the desktop geometry
    """
    rect = QApplication.desktop().screenGeometry()
    desktop_width = rect.width()

    widths = (1920, 1600, 1280, 1024, 800, 640, 200)
    for w in widths:
        if w < desktop_width:
            return w


def run_alarm_panel(ns):
    app = create_gui_app(sys.argv)
    init_gui(app, use_splash=not ns.nosplash)
    # Process events, let everything render, even on the slow machines!
    process_qt_events(timeout=10000)
    # We might want to connect directly to the gui server
    if ns.host and ns.port:
        success = get_network().connectToServerDirectly(
            username=ns.username, hostname=ns.host, port=ns.port)
    else:
        # Connect to the GUI Server via dialog
        success = get_network().connectToServer()

    if success:
        width = check_desktop_width()

        panel = AlarmPanel()
        panel.toolbar.setVisible(True)
        panel.resize(*(width, 500))
        panel.show()

        # Get the panel wrangler singleton and close the splash screen
        wrangler = get_panel_wrangler()
        if wrangler.splash is not None:
            wrangler.splash.close()
            wrangler.splash = None

        app.exec()
        app.deleteLater()
        sys.exit()
    else:
        # If we are not successful in connection, we don't leave a remnant!
        app.quit()


def main():
    ap = argparse.ArgumentParser(description='Karabo Cinema')
    ap.add_argument('-host', '--host', type=str,
                    help='The hostname of the gui server to connect')
    ap.add_argument('-port', '--port', type=int,
                    help='The port number of the gui server to connect')
    ap.add_argument('-username', '--username', type=str, default='admin',
                    help='The user name. Only used when specifying host and '
                         'port. The default user name is `admin`')
    ap.add_argument('-nosplash', '--nosplash', action='store_true')
    run_alarm_panel(ap.parse_args())


if __name__ == '__main__':
    main()
