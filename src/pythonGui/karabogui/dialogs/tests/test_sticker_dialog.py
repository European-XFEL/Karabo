from unittest import main, mock

from qtpy.QtGui import QColor, QFont
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import StickerModel
from karabogui.dialogs.api import StickerDialog
from karabogui.testing import GuiTestCase


class TestStickerDialog(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.model = StickerModel(
            text="XFEL",
            width=100, height=200)
        self.dialog = StickerDialog(self.model)

    def test_dialog_basic(self):
        """Test the basics of the sticker dialog"""
        self.assertNotEqual(self.model, self.dialog.model)
        self.assertEqual(self.dialog.styleSheet(), "")

        text_widget = self.dialog.leText
        self.assertEqual(text_widget.width(), self.model.width)
        self.assertEqual(text_widget.height(), self.model.height)
        self.assertEqual(text_widget.toPlainText(), self.model.text)

        self.assertEqual(self.dialog.text_font.toString(),
                         "Source Sans Pro,10,-1,5,50,0,0,0,0,0")

        # Only the internal dialog model is modified
        text_widget.setPlainText("XFEL2")
        self.assertEqual(self.model.text, "XFEL")
        self.assertEqual(self.dialog.model.text, "XFEL2")

        path = "karabogui.dialogs.sticker_dialog.FontDialog"
        with mock.patch(path) as d:
            font = QFont()
            font.setPointSize(20)
            d().qfont = font
            d().exec.return_value = QDialog.Accepted
            self.click(self.dialog.pbFont)
            self.assertEqual(self.dialog.model.font, font.toString())

        self.assertEqual(self.dialog.pbBackground.isEnabled(), True)
        self.assertEqual(self.dialog.model.background, "white")
        self.click(self.dialog.cbBackground)
        self.assertEqual(self.dialog.pbBackground.isEnabled(), False)
        self.assertEqual(self.dialog.model.background, "transparent")

        # Enable again to change background
        self.click(self.dialog.cbBackground)
        path = "karabogui.dialogs.sticker_dialog.QColorDialog"
        with mock.patch(path) as d:
            c = QColor(255, 0, 0)
            d.getColor.return_value = c
            self.click(self.dialog.pbBackground)
            self.assertEqual(self.dialog.model.background, c.name())
            self.assertNotEqual(self.dialog.model.background, "transparent")

        # Change the foreground
        self.assertEqual(self.dialog.model.foreground, "")
        path = "karabogui.dialogs.sticker_dialog.QColorDialog"
        with mock.patch(path) as d:
            c = QColor(0, 255, 0)
            d.getColor.return_value = c
            self.click(self.dialog.pbTextColor)
            self.assertEqual(self.dialog.model.foreground, c.name())


if __name__ == "__main__":
    main()
