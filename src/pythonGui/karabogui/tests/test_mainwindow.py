from karabogui.mainwindow import MainWindow
from karabogui.testing import GuiTestCase


class TestCase(GuiTestCase):

    def test_mainwindow(self):
        """Small test for verifying that at least the imports are right"""
        MainWindow()
