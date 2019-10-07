import os.path as op
import sys

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QPixmap, QSplashScreen

from karabogui.events import broadcast_event, KaraboEvent
from karabogui.programs.base import create_gui_app, excepthook, init_gui


def run_gui(args):
    app = create_gui_app(args)
    splash_path = op.join(op.dirname(__file__), '..', "icons", "splash.png")
    splash_img = QPixmap(splash_path)
    splash = QSplashScreen(splash_img, Qt.WindowStaysOnTopHint)
    splash.setMask(splash_img.mask())
    splash.show()
    app.processEvents()

    # This is needed to make the splash screen show up...
    splash.showMessage(" ")
    app.processEvents()

    # some final initialization
    init_gui(app, splash)

    # Make the main window
    broadcast_event(KaraboEvent.CreateMainWindow, {})

    # then start the event loop
    app.exec_()
    app.deleteLater()
    sys.exit()


def main():
    sys.excepthook = excepthook
    run_gui(sys.argv)


if __name__ == '__main__':
    main()
