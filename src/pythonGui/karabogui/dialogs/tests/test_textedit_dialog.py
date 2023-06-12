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

from qtpy.QtGui import QColor, QFont
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import (
    DeviceSceneLinkModel, LabelModel, SceneLinkModel, WebLinkModel)
from karabogui.dialogs.textdialog import TextDialog
from karabogui.fonts import get_alias_from_font
from karabogui.testing import GuiTestCase


class TestTextDialog(GuiTestCase):

    def test_basics(self):
        self.check_model(LabelModel(), alignment=True)
        self.check_model(WebLinkModel())
        self.check_model(SceneLinkModel())
        self.check_model(DeviceSceneLinkModel())

    def check_model(self, model, alignment=False):
        self.model = model
        self.dialog = TextDialog(label_model=self.model, alignment=alignment)
        self.assertFalse(self.dialog.cbBackground.isChecked())

        # Set background
        self.assertEqual(self.dialog.label_model.background, "transparent")
        self.click(self.dialog.cbBackground)
        self.process_qt_events()
        self.assertTrue(self.dialog.cbBackground.isChecked())
        self.assertEqual(self.dialog.label_model.background, "transparent")
        with mock.patch("karabogui.dialogs.textdialog.QColorDialog") as d:
            brush_color = QColor(211, 0, 33, 255)
            d.getColor.return_value = brush_color
            self.click(self.dialog.pbBackground)
            self.process_qt_events()
            self.assertEqual(self.dialog.label_model.background,
                             brush_color.name())
        # Erase background again
        self.click(self.dialog.cbBackground)
        self.process_qt_events()
        self.assertFalse(self.dialog.cbBackground.isChecked())
        self.assertEqual(self.dialog.label_model.background, "transparent")

        # Change foreground color (text)
        with mock.patch("karabogui.dialogs.textdialog.QColorDialog") as d:
            brush_color = QColor(0, 0, 10, 255)
            d.getColor.return_value = brush_color
            self.click(self.dialog.pbTextColor)
            self.process_qt_events()
            # Protection, we do not want our model to be modifed
            self.assertNotEqual(self.model.foreground, brush_color)
            self.assertEqual(self.dialog.label_model.foreground,
                             brush_color.name())

        # Change text
        self.dialog.leText.setText("NewText")
        self.assertEqual(self.dialog.label_model.text, "NewText")

        # Frame width
        self.click(self.dialog.cbFrameWidth)
        self.process_qt_events()
        self.assertEqual(self.dialog.label_model.frame_width, 0)
        self.dialog.sbFrameWidth.setValue(2)
        self.assertEqual(self.dialog.label_model.frame_width, 2)

        # Change font
        with mock.patch("karabogui.dialogs.textdialog.FontDialog") as d:
            new_font = QFont("Times New Roman")
            d().qfont = new_font
            d().exec.return_value = QDialog.Accepted
            self.click(self.dialog.pbFont)
            self.process_qt_events()
            self.assertEqual(self.dialog.text_font.toString(),
                             new_font.toString())
            self.assertEqual(self.dialog.label_model.font, new_font.toString())

        if alignment:
            self.dialog.cbAlignment.setCurrentIndex(1)
            self.assertEqual(self.dialog.label_model.alignh, 2)
            self.assertEqual(self.dialog.cbAlignment.currentText(),
                             "AlignRight")
            self.dialog.cbAlignment.setCurrentIndex(2)
            self.assertEqual(self.dialog.cbAlignment.currentText(),
                             "AlignHCenter")
            self.assertEqual(self.dialog.label_model.alignh, 4)
            self.dialog.cbAlignment.setCurrentIndex(0)
            self.assertEqual(self.dialog.label_model.alignh, 1)
            self.assertEqual(self.dialog.cbAlignment.currentText(),
                             "AlignLeft")
        else:
            self.assertEqual(self.dialog.cbAlignment.currentIndex(), 2)
            self.assertEqual(self.dialog.cbAlignment.currentText(),
                             "AlignHCenter")

    def test_set_text_font_button(self):
        """
        The Font buttons text and font family is updated, on changing the font
        selection. Font size remains same.
        """
        model = LabelModel()
        dialog = TextDialog(model)

        font = QFont()
        font.setFamily("Monospaced")
        font.setPointSize(23)
        dialog.text_font = font
        dialog.set_text_font_button()
        button_font = dialog.pbFont.font()

        assert button_font.family() == "Monospaced"
        assert button_font.pointSize() == 10
        text = get_alias_from_font("Monospaced") + ", 23pt"
        assert dialog.pbFont.text() == text

        font.setFamily("Sans Serif")
        font.setPointSize(30)
        dialog.text_font = font
        dialog.set_text_font_button()

        button_font = dialog.pbFont.font()
        assert button_font.family() == "Sans Serif"
        assert button_font.pointSize() == 10
        text = get_alias_from_font("Sans Serif") + ", 30pt"
        assert dialog.pbFont.text() == text

        font.setStrikeOut(True)
        font.setBold(True)
        font.setItalic(True)
        dialog.text_font = font
        dialog.set_text_font_button()

        button_font = dialog.pbFont.font()
        assert button_font.strikeOut()
        assert button_font.bold()
        assert button_font.italic()

        font.setStrikeOut(False)
        font.setBold(False)
        font.setItalic(False)
        dialog.text_font = font
        dialog.set_text_font_button()

        button_font = dialog.pbFont.font()
        assert not button_font.strikeOut()
        assert not button_font.bold()
        assert not button_font.italic()


if __name__ == "__main__":
    main()
