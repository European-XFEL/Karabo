from PyQt5.QtGui import QFont, QFontDatabase

from karabogui.fonts import FONT_FILENAMES, get_alias_from_font
from karabogui.testing import GuiTestCase

from ..font_dialog import FontDialog


class TestFontDialog(GuiTestCase):

    def setUp(self):
        super(TestFontDialog, self).setUp()
        # Add fonts
        for font_file in FONT_FILENAMES:
            QFontDatabase.addApplicationFont(font_file)

        self.qfont = qfont = QFont("Source Sans Pro")
        qfont.setPointSize(20)
        qfont.setBold(True)
        qfont.setItalic(True)
        qfont.setStrikeOut(True)
        qfont.setUnderline(True)

        self.dialog = FontDialog(self.qfont)

    def tearDown(self):
        self.dialog.destroy()

    def test_basics(self):
        self._assert_qfont(self.qfont)

    def test_font_change(self):
        family = "Source Serif Pro"
        qfont = QFont(self.qfont)
        qfont.setFamily(family)

        alias = get_alias_from_font(family)
        self.dialog.font_combobox.setCurrentText(alias)
        self._assert_qfont(qfont)

    def test_font_size_change(self):
        size = 30
        qfont = QFont(self.qfont)
        qfont.setPointSize(size)

        self.dialog.font_size_combobox.setCurrentText(str(size))
        self._assert_qfont(qfont)

    def test_bold_change(self):
        enabled = not self.qfont.bold()
        qfont = QFont(self.qfont)
        qfont.setBold(enabled)

        self.dialog.bold_checkbox.setChecked(enabled)
        self._assert_qfont(qfont)

    def test_italic_change(self):
        enabled = not self.qfont.italic()
        qfont = QFont(self.qfont)
        qfont.setItalic(enabled)

        self.dialog.italic_checkbox.setChecked(enabled)
        self._assert_qfont(qfont)

    def test_strikeout_change(self):
        enabled = not self.qfont.strikeOut()
        qfont = QFont(self.qfont)
        qfont.setStrikeOut(enabled)

        self.dialog.strikeout_checkbox.setChecked(enabled)
        self._assert_qfont(qfont)

    def test_underline_change(self):
        enabled = not self.qfont.underline()
        qfont = QFont(self.qfont)
        qfont.setUnderline(enabled)

        self.dialog.underline_checkbox.setChecked(enabled)
        self._assert_qfont(qfont)

    def _assert_qfont(self, qfont):
        dialog = self.dialog
        self.assertEqual(dialog.qfont, qfont)
        self.assertEqual(dialog.font_combobox.currentText(),
                         get_alias_from_font(qfont.family()))
        self.assertEqual(dialog.font_size_combobox.currentText(),
                         str(qfont.pointSize()))
        self.assertEqual(dialog.bold_checkbox.isChecked(),
                         qfont.bold())
        self.assertEqual(dialog.italic_checkbox.isChecked(),
                         qfont.italic())
        self.assertEqual(dialog.strikeout_checkbox.isChecked(),
                         qfont.strikeOut())
        self.assertEqual(dialog.underline_checkbox.isChecked(),
                         qfont.underline())

        # Check if font string is equal to the font
        new_font = QFont()
        new_font.fromString(qfont.toString())
        self.assertEqual(dialog.qfont, new_font)