import sys

from karabogui.events import broadcast_event, KaraboEvent
from karabogui.programs.base import create_gui_app, init_gui


def run_gui(args):
    app = create_gui_app(args)
    # some final initialization
    init_gui(app, use_splash=True)

    # Make the main window
    broadcast_event(KaraboEvent.CreateMainWindow, {})

    # then start the event loop
    app.exec_()
    app.deleteLater()
    sys.exit()


def main():
    run_gui(sys.argv)


if __name__ == '__main__':
    main()
