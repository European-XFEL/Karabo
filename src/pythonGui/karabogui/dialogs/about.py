# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import os.path as op

from qtpy import uic
from qtpy.QtCore import Qt
from qtpy.QtGui import QPixmap
from qtpy.QtWidgets import QDialog

from karabogui import const, icons
from karabogui.singletons.api import get_manager, get_network

from .utils import get_dialog_ui


class _PatternMatcher:
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
        super().__init__(parent)
        uic.loadUi(get_dialog_ui('about.ui'), self)
        self.setAttribute(Qt.WA_DeleteOnClose)

        image_path = op.join(op.dirname(icons.__file__), 'tunnel.png')
        tunnel_img = QPixmap(image_path)
        self.imgLabel.setPixmap(tunnel_img)

        txt = self.aboutText.text()
        self.aboutText.setText(
            txt.replace('$KARABO_VERSION', const.GUI_VERSION_DETAILED, 1))

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
        return super().keyPressEvent(event)
