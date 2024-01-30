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

from qtpy.QtGui import QColor

from karabogui.graph.plots.api import GraphViewDialog
from karabogui.testing import GuiTestCase


class TestDialogs(GuiTestCase):

    def test_view_dialog(self):
        # Test no config dialog
        dialog = GraphViewDialog()
        assert dialog.graph_bg_color == "transparent"
        assert not dialog.ui_cb_background.isChecked()

        settings = dialog.settings
        assert settings["background"] == "transparent"
        assert settings["title"] == ""

        path = "karabogui.graph.plots.dialogs.view.QColorDialog"
        with mock.patch(path) as color:
            c = QColor(0, 0, 255)
            color.getColor.return_value = c

            # Enable clicking dialog background
            self.click(dialog.ui_cb_background)
            self.click(dialog.ui_pb_background)
            self.process_qt_events()
            assert dialog.graph_bg_color == c.name()

            settings = dialog.settings
            assert settings["background"] == c.name()
            assert settings["title"] == ""

            # Disable background again
            self.click(dialog.ui_cb_background)
            settings = dialog.settings
            assert settings["background"] == "transparent"
            assert settings["title"] == ""

            title = "XRAY"
            dialog.ui_title.setText(title)
            settings = dialog.settings
            assert settings["background"] == "transparent"
            assert settings["title"] == title


if __name__ == "__main__":
    main()
