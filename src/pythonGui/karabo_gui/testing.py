import sys
import unittest
import unittest.mock

from PyQt4.QtGui import QApplication

from karabo_gui import icons


class GuiTestCase(unittest.TestCase):
    """ A convenient base class for gui test cases
    """

    def setUp(self):
        app = QApplication.instance()
        if app is None:
            app = QApplication(sys.argv)
        self.app = app

        icons.init()

        # Mock the stupid global mainwindow object
        window = unittest.mock.patch('karabo_gui.gui.window')
        self.gui_window = window.start()
        self.gui_window.signalGlobalAccessLevelChanged.connect()
        self.gui_window.signalGlobalAccessLevelChanged.disconnect()
