from unittest import main, mock

from qtpy.QtGui import QColor, QFont
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import (
    LabelModel, SceneLinkModel, WebLinkModel)
from karabogui.dialogs.textdialog import TextDialog
from karabogui.testing import GuiTestCase


class TestTextDialog(GuiTestCase):

    def test_basics(self):
        self.check_model(LabelModel(), alignment=True)
        self.check_model(WebLinkModel())
        self.check_model(SceneLinkModel())

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


if __name__ == "__main__":
    main()
