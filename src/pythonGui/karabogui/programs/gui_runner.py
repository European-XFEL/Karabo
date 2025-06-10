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

from karabo.common.scenemodel.api import set_scene_reader
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.programs.base import create_gui_app, init_gui
from karabogui.singletons.api import get_config


def run_gui(ns):
    app = create_gui_app([])

    set_scene_reader(lazy=True)
    # Set the development mode
    get_config()["development"] = ns.dev

    # some final initialization
    init_gui(app, use_splash=True)

    # Make the main window
    broadcast_event(KaraboEvent.CreateMainWindow, {})

    # then start the event loop
    app.exec()
    app.deleteLater()
    sys.exit()


def main():
    ap = argparse.ArgumentParser(description='Karabo GUI')
    ap.add_argument('-dev', '--dev', action='store_true')
    run_gui(ap.parse_args())


if __name__ == '__main__':
    main()
