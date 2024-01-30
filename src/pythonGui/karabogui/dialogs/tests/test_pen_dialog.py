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

        assert pen.miterLimit() == dialog.dsbStrokeMiterLimit.value()
        # We have a Qt.SolidPattern
        assert dialog.gbFill.isChecked() is True

        # 2. Check to initialize without brush
        pen = QPen()
        pen.setStyle(Qt.CustomDashLine)
        dialog = PenDialog(pen)
        assert dialog is not None
        assert dialog.pen.style() == Qt.CustomDashLine
        assert dialog.wDashType.penStyle() == 1

        # 3. Initialize with pen and brush, brush is no brush
        pen = QPen()
        pen.setColor(QColor(0, 255, 0))
        pen.setWidth(2)
        brush = QBrush()
        brush.setStyle(Qt.NoBrush)
        dialog = PenDialog(pen, brush)
        assert dialog.gbFill.isChecked() is False

        with mock.patch("karabogui.dialogs.pen_dialogs.QColorDialog") as d:
            brush_color = QColor(0, 0, 10, 255)
            d.getColor.return_value = brush_color
            dialog.on_pbFillColor_clicked()
            assert dialog.brush.color() == brush_color

        with mock.patch("karabogui.dialogs.pen_dialogs.QColorDialog") as d:
            pen_color = QColor(0, 255, 0, 255)
            d.getColor.return_value = pen_color
            dialog.on_pbStrokeColor_clicked()
            assert dialog.pen.color() == pen_color

        # Note: Check the accept dialog
        # Change to Solid Pattern
        dialog.gbFill.setChecked(True)
        dialog.on_dialog_accepted()
        assert dialog.brush.style() == Qt.SolidPattern
        dialog.sbStrokeWidth.setValue(0)
        dialog.on_dialog_accepted()
        assert dialog.pen.style() == Qt.NoPen
        dialog.sbStrokeWidth.setValue(2)
        dialog.on_dialog_accepted()
        assert dialog.pen.style() != Qt.NoPen
        assert dialog.pen.style() == Qt.SolidLine
        assert dialog.pen.dashOffset() == 0

        dialog.dsbStrokeMiterLimit.setValue(1)
        dialog.on_dialog_accepted()
        assert dialog.pen.miterLimit() == 1


if __name__ == "__main__":
    main()
