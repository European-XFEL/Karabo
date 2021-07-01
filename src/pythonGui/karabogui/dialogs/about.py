import os.path as op

from qtpy import uic
from qtpy.QtCore import Qt
from qtpy.QtGui import QPixmap
from qtpy.QtWidgets import QDialog

from karabogui import icons
from karabogui.singletons.api import get_manager, get_network

from .utils import get_dialog_ui


class _PatternMatcher(object):
    """A tiny state machine which watches for a single pattern.

    Useful for watching for specific key sequences...
    """

    def __init__(self, pattern):
        self.pattern = pattern
        self.index = 0

    def check(self, letter):
        if letter == self.pattern[self.index]:
            self.index += 1
            if self.index == len(self.pattern):
                self.index = 0
                return True
        else:
            self.index = 0

        return False


class AboutDialog(QDialog):
    """The about box for our application.

    NOTE: We watch for "cheat codes" here to enable/disable certain application
    features.
    """

    def __init__(self, parent=None):
        super(AboutDialog, self).__init__(parent)
        uic.loadUi(get_dialog_ui('about.ui'), self)
        self.setAttribute(Qt.WA_DeleteOnClose)

        image_path = op.join(op.dirname(icons.__file__), 'tunnel.png')
        tunnel_img = QPixmap(image_path)
        self.imgLabel.setPixmap(tunnel_img)

        # Pattern matchers for a specific key combos
        self._cheat_codes = {
            _PatternMatcher('chooch'):
                lambda: get_network().togglePerformanceMonitor(),
            _PatternMatcher('bigdata'):
                lambda: get_manager().toggleBigDataPerformanceMonitor()
        }

    def keyPressEvent(self, event):
        text = event.text()
        for matcher, trigger in self._cheat_codes.items():
            if matcher.check(text):
                trigger()
        return super(AboutDialog, self).keyPressEvent(event)
