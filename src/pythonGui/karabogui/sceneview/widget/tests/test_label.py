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

from qtpy.QtCore import QPoint, QRect
from qtpy.QtWidgets import QDialog, QWidget

from karabo.common.scenemodel.api import LabelModel
from karabogui.sceneview.widget.label import LabelWidget
from karabogui.testing import GuiTestCase


class TestLabelWidget(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.model = LabelModel(
            text="Label Text", foreground="black",
            x=0, y=0, width=100, height=100, alignh=1)

        self.main_widget = QWidget()
        self.widget = LabelWidget(model=self.model,
                                  parent=self.main_widget)
        self.main_widget.show()

    def tearDown(self):
        super().tearDown()
        self.main_widget = None
        self.widget = None

    # -----------------------------------------------------------------------
    # Actual tests

    def test_basics(self):
        model_rect = QRect(self.model.x, self.model.y,
                           self.model.width, self.model.height)
        self.assertTrue(self.widget.rect() == model_rect)
        # Actions
        self.assertEqual(len(self.widget.actions()), 1)
        edit_action = self.widget.actions()[0]
        self.assertEqual(edit_action.text(), "Edit Label")

        text_before = "Label Text"
        self.assertEqual(self.widget.text(), text_before)
        self.assertEqual(self.model.text, text_before)

        path = "karabogui.sceneview.widget.label.TextDialog"
        with mock.patch(path) as dialog:
            dialog.exec.return_value = QDialog.Accepted
            text_after = "This is a label text"
            dialog().label_model = LabelModel(text=text_after)
            edit_action.trigger()
            self.assertEqual(self.model.text, text_after)

        # Make sure, basic interface is there
        self.widget.add_proxies(None)
        self.widget.apply_changes()
        self.widget.decline_changes()
        self.widget.destroy()
        self.widget.set_visible(True)
        self.widget.update_global_access_level(None)
        self.assertIsNotNone(self.widget)

        # Geometry
        rect = QRect(2, 10, 2, 10)
        self.assertFalse(self.widget.geometry() == rect)
        self.widget.set_geometry(rect)
        self.assertTrue(self.widget.geometry() == rect)

        # Translation
        self.assertEqual(self.widget.pos(), QPoint(2, 10))
        self.widget.translate(QPoint(10, 0))
        self.assertEqual(self.widget.pos(), QPoint(12, 10))


if __name__ == "__main__":
    main()
