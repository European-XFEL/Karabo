import sys
import unittest

from PyQt4.QtGui import QApplication


class GuiTestCase(unittest.TestCase):
    """ A convenient base class for gui test cases
    """

    def setUp(self):
        app = QApplication.instance()
        if app is None:
            app = QApplication(sys.argv)
        self.app = app
