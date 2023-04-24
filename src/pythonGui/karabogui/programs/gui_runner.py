# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import argparse
import sys

from karabogui.events import KaraboEvent, broadcast_event
from karabogui.programs.base import create_gui_app, init_gui
from karabogui.singletons.api import get_config


def run_gui(ns):
    app = create_gui_app([])

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
