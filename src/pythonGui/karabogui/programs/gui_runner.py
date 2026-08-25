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
from importlib import import_module

from karabo.common.scenemodel.api import set_scene_reader
from karabogui.const import GUI_VERSION_DETAILED
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.programs.base import create_gui_app, init_gui
from karabogui.singletons.api import get_config
from karabogui.util import is_bundled_gui

GUI = "gui"
CINEMA = "cinema"
CONCERT = "concert"
THEATRE = "theatre"

PROGRAM_MODULES = {
    GUI: "karabogui.programs.gui_runner",
    CINEMA: "karabogui.programs.cinema",
    CONCERT: "karabogui.programs.concert",
    THEATRE: "karabogui.programs.theatre",
}


def _get_program_main(program):
    module_name = PROGRAM_MODULES[program]
    return import_module(module_name).main


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


def create_gui_parser():
    parser = argparse.ArgumentParser(description="Karabo GUI")
    parser.add_argument("-dev", "--dev", action="store_true")
    parser.add_argument("-V", "--version", action="version",
                        version=f"%(prog)s {GUI_VERSION_DETAILED}")
    return parser


def main():
    if is_bundled_gui():
        parser = argparse.ArgumentParser(description="Karabo GUI")
        parser.add_argument(
            "program",
            nargs="?",
            default=GUI,
            choices=(GUI, CINEMA, THEATRE, CONCERT),
            help="Choose the Karabo application mode - cinema, theatre or "
                 "concert. The main GUI is launched when no argument is "
                 "provided."
        )
        ns, remaining = parser.parse_known_args()
        program = ns.program

        if program != GUI:
            original_argv = sys.argv[:]
            try:
                sys.argv = [sys.argv[0], *remaining]
                _get_program_main(program)()
            finally:
                sys.argv = original_argv
            return

        run_gui(create_gui_parser().parse_args(remaining))
        return

    run_gui(create_gui_parser().parse_args())


if __name__ == '__main__':
    main()
