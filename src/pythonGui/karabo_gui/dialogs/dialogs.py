#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created on March 19, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from os import path

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot, Qt, QSize
from PyQt4.QtGui import (QDialogButtonBox, QColorDialog, QComboBox, QDialog,
                         QFormLayout, QIcon, QPainter, QPen, QPixmap,
                         QTableWidgetItem)

from karabo.common.api import walk_traits_object
from karabo.common.scenemodel.api import SceneModel, SceneTargetWindow
from karabo_gui.singletons.api import get_project_model


class PenDialog(QDialog):
    linecaps = {Qt.FlatCap: "butt", Qt.SquareCap: "square",
                Qt.RoundCap: "round"}
    linejoins = {Qt.SvgMiterJoin: "miter", Qt.MiterJoin: "miter",
                 Qt.BevelJoin: "bevel", Qt.RoundJoin: "round"}
    pen_styles = [Qt.SolidLine, Qt.DashLine, Qt.DotLine, Qt.DashDotLine,
                  Qt.DashDotDotLine]

    def __init__(self, pen, brush=None):
        QDialog.__init__(self)
        uic.loadUi(path.join(path.dirname(__file__), 'pendialog.ui'), self)

        self.pen = pen
        self.brush = brush

        self.set_color()

        self.slStrokeOpacity.setValue(pen.color().alpha())

        if pen.style() == Qt.NoPen:
            self.sbStrokeWidth.setValue(0)
        else:
            self.sbStrokeWidth.setValue(pen.widthF())

        self.wDashType = PenStyleComboBox()
        self.formLayout.setWidget(3, QFormLayout.FieldRole, self.wDashType)
        self.wDashType.setPenStyle(self._get_style_from_pattern(self.pen))

        self.dsbDashOffset.setValue(self.pen.dashOffset())

        getattr(self, self.linecaps[self.pen.capStyle()] + 'Cap').setChecked(True)
        getattr(self, self.linejoins[self.pen.joinStyle()] + 'Join').setChecked(True)
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

    @pyqtSlot()
    def on_pbStrokeColor_clicked(self):
        self.pen.setColor(QColorDialog.getColor(self.pen.color()))
        self.set_color()

    @pyqtSlot()
    def on_pbFillColor_clicked(self):
        self.brush.setColor(QColorDialog.getColor(self.brush.color()))
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

    def exec_(self):
        result = QDialog.exec_(self)
        if result == QDialog.Accepted:
            strokeColor = self.pen.color()
            strokeColor.setAlpha(self.slStrokeOpacity.value())
            self.pen.setColor(strokeColor)

            if self.sbStrokeWidth.value() == 0:
                self.pen.setStyle(Qt.NoPen)
            else:
                self.pen.setWidth(self.sbStrokeWidth.value())

            # Set dash offset first, because this sets the pen style to
            # Qt.CustomDashLine
            self.pen.setDashOffset(self.dsbDashOffset.value())
            self.pen.setStyle(self.wDashType.penStyle())

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

        return result


class PenStyleComboBox(QComboBox):
    styles = [(Qt.SolidLine, "Solid line"),
              (Qt.DashLine, "Dashed line"),
              (Qt.DotLine, "Dot line"),
              (Qt.DashDotLine, "Dash dot line"),
              (Qt.DashDotDotLine, "Dash dot dot line")]

    def __init__(self, parent=None):
        super(PenStyleComboBox, self).__init__(parent)

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

        mid = self.iconSize().height() / 2.0
        p.drawLine(0, mid, self.iconSize().width(), mid)
        p.end()

        return QIcon(pix)


class SceneLinkDialog(QDialog):
    def __init__(self, model, parent=None):
        super(SceneLinkDialog, self).__init__(parent=parent)
        uic.loadUi(path.join(path.dirname(__file__), 'scenelink.ui'), self)

        self._selectedScene = 0
        self._sceneTargets = self._get_scenelink_targets()
        sceneCombo = self.sceneSelectCombo
        for scene in self._sceneTargets:
            sceneCombo.addItem(scene)

        targetIndex = sceneCombo.findText(model.target)
        if targetIndex > -1:
            self._selectedScene = targetIndex

        sceneCombo.setCurrentIndex(self._selectedScene)

        self._selectedTargetWin = model.target_window
        radioButtons = {
            SceneTargetWindow.MainWindow: self.mainRadio,
            SceneTargetWindow.Dialog: self.dialogRadio
        }
        button = radioButtons.get(self._selectedTargetWin)
        if button:
            button.setChecked(True)

    def _get_scenelink_targets(self):
        project = get_project_model().traits_data_model
        if project is None:
            return []

        collected = set()

        def visitor(obj):
            nonlocal collected
            if isinstance(obj, SceneModel):
                target = "{}:{}:{}".format(obj.simple_name, obj.uuid,
                                           obj.revision)
                collected.add(target)

        walk_traits_object(project, visitor)
        return list(collected)

    @property
    def selectedScene(self):
        return self._sceneTargets[self._selectedScene]

    @property
    def selectedTargetWindow(self):
        return self._selectedTargetWin

    @pyqtSlot(int)
    def on_sceneSelectCombo_currentIndexChanged(self, index):
        self._selectedScene = index

    @pyqtSlot(bool)
    def on_dialogRadio_clicked(self, checked=False):
        if checked:
            self._selectedTargetWin = SceneTargetWindow.Dialog

    @pyqtSlot(bool)
    def on_mainRadio_clicked(self, checked=False):
        if checked:
            self._selectedTargetWin = SceneTargetWindow.MainWindow


class ReplaceDialog(QDialog):

    def __init__(self, devices):
        QDialog.__init__(self)
        uic.loadUi(path.join(path.dirname(__file__), 'replacedialog.ui'), self)

        self.twTable.setRowCount(len(devices))
        for i, d in enumerate(devices):
            item = QTableWidgetItem(d)
            item.setFlags(item.flags() & ~Qt.ItemIsEditable)
            self.twTable.setItem(i, 0, item)

            item = QTableWidgetItem(d)
            self.twTable.setItem(i, 1, item)
            self.twTable.editItem(item)
        self.twTable.itemChanged.connect(self.onItemChanged)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

    def mappedDevices(self):
        """
        A dict with the mapped devices is returned.
        """
        map = {}
        for i in range(self.twTable.rowCount()):
            map[self.twTable.item(i, 0).text()] = self.twTable.item(i, 1).text()
        return map

    @pyqtSlot(str)
    def onItemChanged(self, item):
        if item.column() != 1:
            return
        text = item.text()
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(len(text) > 0)
