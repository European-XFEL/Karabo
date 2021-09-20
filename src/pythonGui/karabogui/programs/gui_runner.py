import sys

from karabogui.events import KaraboEvent, broadcast_event
from karabogui.programs.base import create_gui_app, init_gui


def run_gui():
    app = create_gui_app([])
    # some final initialization
    init_gui(app, use_splash=True)

    # Make the main window
    broadcast_event(KaraboEvent.CreateMainWindow, {})

    # then start the event loop
    app.exec_()
    app.deleteLater()
    sys.exit()


def main():
    run_gui()


if __name__ == '__main__':
    main()
