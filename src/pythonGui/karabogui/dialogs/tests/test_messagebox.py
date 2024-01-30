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

from qtpy.QtCore import QPoint
from qtpy.QtWidgets import QMessageBox

from karabogui.dialogs.api import KaraboMessageBox
from karabogui.testing import GuiTestCase


class TestMessageBox(GuiTestCase):

    def test_basic(self):
        """Check the basics of the Karabo Messagebox"""
        box_error = self._create_messagebox(
            QMessageBox.Critical,
            "Caution: Pump broke",
            "Temperature to high")

        box_info = self._create_messagebox(
            QMessageBox.Information,
            "New device online",
            "The new motor arrived in the experimental hall")

        assert len(box_info.existing) == 2
        assert len(box_error.existing) == 2

        m = "Temperature to high"
        assert box_error.details_textedit.toPlainText() == m
        assert box_error._is_showing_details is False
        self.click(box_error.show_details_button)
        self.process_qt_events()
        assert box_error._is_showing_details is True
        assert box_info.text_label.text() == "New device online"
        box_error.close()
        assert len(box_error.existing) == 1
        assert len(box_info.existing) == 1

        # Close same again, graceful
        box_error.close()

        box_warning = self._create_messagebox(
            QMessageBox.Warning,
            "Take care!")
        assert len(box_warning.existing) == 2

        with mock.patch("karabogui.dialogs.messagebox.QMenu") as m:
            box_warning._show_context_menu(QPoint(0, 0))
            m.assert_called_once()

    def _create_messagebox(self, icon, text, detail=None):
        """Create a messagebox with `icon`, `text` and `detail`"""
        mbox = KaraboMessageBox()
        mbox.setIcon(icon)
        mbox.setText(text)
        if detail is not None:
            mbox.setDetailedText(detail)
        mbox.update()
        return mbox


if __name__ == "__main__":
    main()
