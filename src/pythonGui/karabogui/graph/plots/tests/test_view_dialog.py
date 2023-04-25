# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest import main, mock

from qtpy.QtGui import QColor

from karabogui.graph.plots.api import GraphViewDialog
from karabogui.testing import GuiTestCase


class TestDialogs(GuiTestCase):

    def test_view_dialog(self):
        # Test no config dialog
        dialog = GraphViewDialog()
        self.assertEqual(dialog.graph_bg_color, "transparent")
        self.assertFalse(dialog.ui_cb_background.isChecked())

        settings = dialog.settings
        self.assertEqual(settings["background"], "transparent")
        self.assertEqual(settings["title"], "")

        path = "karabogui.graph.plots.dialogs.view.QColorDialog"
        with mock.patch(path) as color:
            c = QColor(0, 0, 255)
            color.getColor.return_value = c

            # Enable clicking dialog background
            self.click(dialog.ui_cb_background)
            self.click(dialog.ui_pb_background)
            self.process_qt_events()
            self.assertEqual(dialog.graph_bg_color, c.name())

            settings = dialog.settings
            self.assertEqual(settings["background"], c.name())
            self.assertEqual(settings["title"], "")

            # Disable background again
            self.click(dialog.ui_cb_background)
            settings = dialog.settings
            self.assertEqual(settings["background"], "transparent")
            self.assertEqual(settings["title"], "")

            title = "XRAY"
            dialog.ui_title.setText(title)
            settings = dialog.settings
            self.assertEqual(settings["background"], "transparent")
            self.assertEqual(settings["title"], title)


if __name__ == "__main__":
    main()
