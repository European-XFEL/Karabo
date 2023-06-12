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
from unittest import main, mock

from qtpy.QtCore import QEvent, Qt
from qtpy.QtGui import QKeyEvent

from karabogui.dialogs.api import AboutDialog
from karabogui.testing import GuiTestCase, singletons


class TestAboutDialog(GuiTestCase):

    def keyKeyPressEvent(self, dialog, key):
        event = QKeyEvent(QEvent.KeyPress, 0, Qt.NoModifier,
                          key, False, 1)
        dialog.keyPressEvent(event)

    def test_basic_dialog(self):
        network = mock.Mock()
        manager = mock.Mock()
        with singletons(network=network, manager=manager):
            dialog = AboutDialog()
            for key in "chooch":
                self.keyKeyPressEvent(dialog, key)
            network.togglePerformanceMonitor.assert_called_once()

            network.reset_mock()
            for key in "cshooch":
                self.keyKeyPressEvent(dialog, key)
            network.togglePerformanceMonitor.assert_not_called()

            network.reset_mock()
            for key in "achooch":
                self.keyKeyPressEvent(dialog, key)
            network.togglePerformanceMonitor.assert_called_once()

            for key in "bigdata":
                self.keyKeyPressEvent(dialog, key)
            manager.toggleBigDataPerformanceMonitor.assert_called_once()

            manager.reset_mock()
            for key in "bigndata":
                self.keyKeyPressEvent(dialog, key)
            manager.toggleBigDataPerformanceMonitor.assert_not_called()


if __name__ == "__main__":
    main()
