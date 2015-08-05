import os.path as op
import sys

# assure sip api is set first
import sip
sip.setapi("QString", 2)
sip.setapi("QVariant", 2)
sip.setapi("QUrl", 2)

sys.karabo_gui = True
import gui

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QApplication, QSplashScreen, QPixmap


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

    gui.init(app)

    splash.finish(gui.window)
    sys.exit(app.exec_())

if __name__ == '__main__':
    sys.excepthook = gui.excepthook
    run_gui(sys.argv)
