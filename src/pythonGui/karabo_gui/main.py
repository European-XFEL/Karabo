import os.path as op
import sys
from traceback import print_exception, format_exception

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QApplication, QMessageBox, QPixmap, QSplashScreen

from karabo_gui.gui import init_gui
from karabo_gui.singletons.api import get_network, get_panel_wrangler


def excepthook(exc_type, value, traceback):
    print_exception(exc_type, value, traceback)
    icon = getattr(value, "icon", QMessageBox.Critical)
    title = getattr(value, "title", exc_type.__name__)
    message = getattr(value, "message",
                      "{}: {}".format(exc_type.__name__, value))
    mb = QMessageBox(icon, title, "{}\n{}\n\n".format(message, " " * 300))
    text = "".join(format_exception(exc_type, value, traceback))
    mb.setDetailedText(text)
    # NOTE: Uncomment to show exceptions in a message box
    # mb.exec_()
    try:
        network = get_network()
        network.onError(text)
    except Exception:
        print("could not send exception to network")


def run_gui(args):
    app = QApplication(args)
    splash_path = op.join(op.dirname(__file__), "icons", "xfel_logo.png")
    splash_img = QPixmap(splash_path)
    splash = QSplashScreen(splash_img, Qt.WindowStaysOnTopHint)
    splash.show()
    app.processEvents()

    # This is needed to make the splash screen show up...
    splash.showMessage(" ")
    app.processEvents()

    init_gui(app)

    splash.finish(get_panel_wrangler().main_window)
    sys.exit(app.exec_())


def main():
    sys.excepthook = excepthook
    run_gui(sys.argv)


if __name__ == '__main__':
    main()
