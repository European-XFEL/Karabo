from unittest import main, mock

from qtpy.QtCore import Qt
from qtpy.QtGui import QBrush, QColor, QPen

from karabogui.dialogs.api import PenDialog
from karabogui.testing import GuiTestCase


class TestLoginDialog(GuiTestCase):

    def test_basic_dialog(self):
        """Check the basics of the pen dialog"""
        # 1. First initialize with pen and brush, brush is solid
        pen = QPen()
        pen.setColor(QColor(0, 255, 0))
        pen.setWidth(2)
        brush = QBrush()
        brush.setStyle(Qt.SolidPattern)
        dialog = PenDialog(pen, brush)

        self.assertEqual(pen.miterLimit(), dialog.dsbStrokeMiterLimit.value())
        # We have a Qt.SolidPattern
        self.assertEqual(dialog.gbFill.isChecked(), True)

        # 2. Check to initialize without brush
        pen = QPen()
        pen.setStyle(Qt.CustomDashLine)
        dialog = PenDialog(pen)
        self.assertIsNotNone(dialog)
        self.assertEqual(dialog.pen.style(), Qt.CustomDashLine)
        self.assertEqual(dialog.wDashType.penStyle(), 1)

        # 3. Initialize with pen and brush, brush is no brush
        pen = QPen()
        pen.setColor(QColor(0, 255, 0))
        pen.setWidth(2)
        brush = QBrush()
        brush.setStyle(Qt.NoBrush)
        dialog = PenDialog(pen, brush)
        self.assertEqual(dialog.gbFill.isChecked(), False)

        with mock.patch("karabogui.dialogs.pen_dialogs.QColorDialog") as d:
            brush_color = QColor(0, 0, 10, 255)
            d.getColor.return_value = brush_color
            dialog.on_pbFillColor_clicked()
            self.assertEqual(dialog.brush.color(), brush_color)

        with mock.patch("karabogui.dialogs.pen_dialogs.QColorDialog") as d:
            pen_color = QColor(0, 255, 0, 255)
            d.getColor.return_value = pen_color
            dialog.on_pbStrokeColor_clicked()
            self.assertEqual(dialog.pen.color(), pen_color)

        # Note: Check the accept dialog
        # Change to Solid Pattern
        dialog.gbFill.setChecked(True)
        dialog.on_dialog_accepted()
        self.assertEqual(dialog.brush.style(), Qt.SolidPattern)
        dialog.sbStrokeWidth.setValue(0)
        dialog.on_dialog_accepted()
        self.assertEqual(dialog.pen.style(), Qt.NoPen)
        dialog.sbStrokeWidth.setValue(2)
        dialog.on_dialog_accepted()
        self.assertNotEqual(dialog.pen.style(), Qt.NoPen)
        self.assertEqual(dialog.pen.style(), Qt.SolidLine)
        self.assertEqual(dialog.pen.dashOffset(), 0)

        dialog.dsbStrokeMiterLimit.setValue(1)
        dialog.on_dialog_accepted()
        self.assertEqual(dialog.pen.miterLimit(), 1)


if __name__ == "__main__":
    main()