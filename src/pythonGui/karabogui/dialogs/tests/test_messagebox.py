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

        self.assertEqual(len(box_info.existing), 2)
        self.assertEqual(len(box_error.existing), 2)

        self.assertEqual(box_error.details_textedit.toPlainText(),
                         "Temperature to high")
        self.assertEqual(box_error._is_showing_details, False)
        self.click(box_error.show_details_button)
        self.process_qt_events()
        self.assertEqual(box_error._is_showing_details, True)
        self.assertEqual(box_info.text_label.text(),
                         "New device online")
        box_error.close()
        self.assertEqual(len(box_error.existing), 1)
        self.assertEqual(len(box_info.existing), 1)

        # Close same again, graceful
        box_error.close()

        box_warning = self._create_messagebox(
            QMessageBox.Warning,
            "Take care!")
        self.assertEqual(len(box_warning.existing), 2)

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