#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created on March 19, 2014
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
#############################################################################
from qtpy import uic
from qtpy.QtCore import QSize, Qt, Slot
from qtpy.QtGui import QIcon, QPainter, QPen, QPixmap
from qtpy.QtWidgets import QColorDialog, QComboBox, QDialog, QFormLayout

from .utils import get_dialog_ui


class PenDialog(QDialog):
    linecaps = {Qt.FlatCap: "butt", Qt.SquareCap: "square",
                Qt.RoundCap: "round"}
    linejoins = {Qt.SvgMiterJoin: "miter", Qt.MiterJoin: "miter",
                 Qt.BevelJoin: "bevel", Qt.RoundJoin: "round"}
    pen_styles = [Qt.SolidLine, Qt.DashLine, Qt.DotLine, Qt.DashDotLine,
                  Qt.DashDotDotLine]

    def __init__(self, pen, brush=None, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui('pendialog.ui'), self)
        self.pen = pen
        self.brush = brush

        self.set_color()

        self.slStrokeOpacity.setValue(pen.color().alpha())

        if pen.style() == Qt.NoPen:
            self.sbStrokeWidth.setValue(0)
        else:
            self.sbStrokeWidth.setValue(pen.width())

        self.wDashType = PenStyleComboBox()
        self.formLayout.setWidget(3, QFormLayout.FieldRole, self.wDashType)
        self.wDashType.setPenStyle(self._get_style_from_pattern(self.pen))

        self.dsbDashOffset.setValue(self.pen.dashOffset())

        cap_button = getattr(self, self.linecaps[self.pen.capStyle()] + 'Cap')
        cap_button.setChecked(True)
        join_name = self.linejoins[self.pen.joinStyle()] + 'Join'
        join_button = getattr(self, join_name)
        join_button.setChecked(True)
        self.dsbStrokeMiterLimit.setValue(self.pen.miterLimit())

        self.setBrushWidgets()

    def _get_style_from_pattern(self, pen):
        """ The pen style for the given ``pen`` is returned. If it is
            ``Qt.CustomDashLine`` the dash pattern is compared.
        """
        if pen.style() == Qt.CustomDashLine:
            p = QPen()
            for style in self.pen_styles:
                p.setStyle(style)
                if p.dashPattern() == pen.dashPattern():
                    return style
        return pen.style()

    @Slot()
    def on_pbStrokeColor_clicked(self):
        color = QColorDialog.getColor(self.pen.color())
        if color.isValid():
            self.pen.setColor(color)
            self.set_color()

    @Slot()
    def on_pbFillColor_clicked(self):
        # The button is now only visible when `self.brush` is not None.
        # Just in case, do nothing.
        if self.brush is None:
            return

        color = QColorDialog.getColor(self.brush.color())
        if color.isValid():
            self.brush.setColor(color)
            self.set_color()

    def set_color(self):
        p = QPixmap(32, 16)
        p.fill(self.pen.color())
        self.pbStrokeColor.setIcon(QIcon(p))

        if self.brush is not None:
            p.fill(self.brush.color())
            self.pbFillColor.setIcon(QIcon(p))

    def setBrushWidgets(self):
        if self.brush is None:
            # Hide brush related widgets
            self.gbFill.setHidden(True)
            self.adjustSize()
        else:
            if self.brush.style() == Qt.SolidPattern:
                self.gbFill.setChecked(True)
            else:
                self.gbFill.setChecked(False)
            self.slFillOpacity.setValue(self.brush.color().alpha())

    def on_dialog_accepted(self):
        strokeColor = self.pen.color()
        strokeColor.setAlpha(self.slStrokeOpacity.value())
        self.pen.setColor(strokeColor)

        if self.sbStrokeWidth.value() == 0:
            self.pen.setStyle(Qt.NoPen)
        else:
            self.pen.setWidth(self.sbStrokeWidth.value())
            # Set style first!
            pen_style = self.wDashType.penStyle()
            self.pen.setStyle(pen_style)

            if pen_style is not Qt.SolidLine:
                self.pen.setDashOffset(self.dsbDashOffset.value())

        for k, v in self.linecaps.items():
            if getattr(self, v + 'Cap').isChecked():
                self.pen.setCapStyle(k)
        for k, v in self.linejoins.items():
            if getattr(self, v + 'Join').isChecked():
                self.pen.setJoinStyle(k)

        self.pen.setMiterLimit(self.dsbStrokeMiterLimit.value())
        if self.brush is not None:
            if not self.gbFill.isChecked():
                self.brush.setStyle(Qt.NoBrush)
            else:
                self.brush.setStyle(Qt.SolidPattern)
                fillColor = self.brush.color()
                fillColor.setAlpha(self.slFillOpacity.value())
                self.brush.setColor(fillColor)

    def exec(self):
        result = QDialog.exec(self)
        if result == QDialog.Accepted:
            self.on_dialog_accepted()
        return result


class PenStyleComboBox(QComboBox):
    styles = [(Qt.SolidLine, "Solid line"),
              (Qt.DashLine, "Dashed line"),
              (Qt.DotLine, "Dot line"),
              (Qt.DashDotLine, "Dash dot line"),
              (Qt.DashDotDotLine, "Dash dot dot line")]

    def __init__(self, parent=None):
        super().__init__(parent)

        self.setIconSize(QSize(32, 12))
        for s in self.styles:
            style = s[0]
            name = s[1]
            self.addItem(self.iconForPen(style), name, style)

    def penStyle(self):
        return self.styles[self.currentIndex()][0]

    def setPenStyle(self, style):
        id = self.findData(style)
        if id == -1:
            id = 0
        self.setCurrentIndex(id)

    def iconForPen(self, style):
        pix = QPixmap(self.iconSize())
        p = QPainter()
        pix.fill(Qt.transparent)

        p.begin(pix)
        pen = QPen(style)
        pen.setWidth(2)
        p.setPen(pen)

        mid = int(self.iconSize().height() / 2)
        p.drawLine(0, mid, int(self.iconSize().width()), mid)
        p.end()

        return QIcon(pix)
