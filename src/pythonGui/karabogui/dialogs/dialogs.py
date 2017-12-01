#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created on March 19, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import QSize, Qt, pyqtSlot
from PyQt4.QtGui import (QColorDialog, QComboBox, QDialog, QDialogButtonBox,
                         QDoubleValidator, QFormLayout, QIcon, QPainter, QPen,
                         QPixmap, QTableWidgetItem)

from karabo.common.api import walk_traits_object
from karabo.common.scenemodel.api import SceneModel, SceneTargetWindow
from karabogui import icons
from karabogui.singletons.api import get_network, get_project_model


class _PatternMatcher(object):
    """A tiny state machine which watches for a single pattern.

    Useful for watching for specific key sequences...
    """
    def __init__(self, pattern):
        self.pattern = pattern
        self.index = 0

    def check(self, letter):
        if letter == self.pattern[self.index]:
            self.index += 1
            if self.index == len(self.pattern):
                self.index = 0
                return True
        else:
            self.index = 0

        return False


class AboutDialog(QDialog):
    """The about box for our application.

    NOTE: We watch for "cheat codes" here to enable/disable certain application
    features.
    """
    def __init__(self, parent=None):
        super(AboutDialog, self).__init__(parent)
        uic.loadUi(op.join(op.dirname(__file__), 'about.ui'), self)

        splash_path = op.join(op.dirname(icons.__file__), 'splash.png')
        splash_img = QPixmap(splash_path)
        self.imgLabel.setPixmap(splash_img)

        # Pattern matchers for a specific key combos
        self._cheat_codes = {
            _PatternMatcher('chooch'):
                lambda: get_network().togglePerformanceMonitor()
        }

    def keyPressEvent(self, event):
        text = event.text()
        for matcher, trigger in self._cheat_codes.items():
            if matcher.check(text):
                trigger()
        return super(AboutDialog, self).keyPressEvent(event)


class LutRangeDialog(QDialog):
    def __init__(self, lut_range, lut_range_full=None, parent=None):
        """A dialog to set the min and max range

        :param lut_range: The range which should be shown
        :parem lut_range_full: The full range according to the data range
        :param parent: The parent of this dialog
        """
        super(LutRangeDialog, self).__init__(parent)
        uic.loadUi(op.join(op.dirname(__file__), "lutrangedialog.ui"), self)
        self.leMin.setValidator(QDoubleValidator())
        self.leMin.textChanged.connect(self._update_button_box)
        self.leMax.setValidator(QDoubleValidator())
        self.leMax.textChanged.connect(self._update_button_box)

        self._update_min_max(lut_range)

        self.lut_range_full = lut_range_full
        self.pbFullRange.setEnabled(self.lut_range_full is not None)
        self.pbFullRange.pressed.connect(self._on_lut_range_full_pressed)

    def _update_min_max(self, min_max_range):
        """The given tuple (min, max) is put into the text fields
        """
        _min, _max = min_max_range
        self.leMin.setText(str(_min))
        self.leMax.setText(str(_max))

    def _update_button_box(self):
        """Only enable Ok button, if title and configuration is set
        """
        _min = self.leMin.text()
        _max = self.leMax.text()
        enabled = (len(_min) > 0 and len(_max) > 0 and
                   float(_min) < float(_max))
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

    @pyqtSlot()
    def _on_lut_range_full_pressed(self):
        """The full range should be shown now
        """
        self._update_min_max(self.lut_range_full)

    @property
    def lut_range(self):
        """Return the LUT transform range tuple: (min, max)
        """
        return (float(self.leMin.text()), float(self.leMax.text()))


class PenDialog(QDialog):
    linecaps = {Qt.FlatCap: "butt", Qt.SquareCap: "square",
                Qt.RoundCap: "round"}
    linejoins = {Qt.SvgMiterJoin: "miter", Qt.MiterJoin: "miter",
                 Qt.BevelJoin: "bevel", Qt.RoundJoin: "round"}
    pen_styles = [Qt.SolidLine, Qt.DashLine, Qt.DotLine, Qt.DashDotLine,
                  Qt.DashDotDotLine]

    def __init__(self, pen, brush=None):
        QDialog.__init__(self)
        uic.loadUi(op.join(op.dirname(__file__), 'pendialog.ui'), self)

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


class ReplaceDialog(QDialog):
    def __init__(self, devices):
        QDialog.__init__(self)
        uic.loadUi(op.join(op.dirname(__file__), 'replacedialog.ui'), self)

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
        retval = {}
        for i in range(self.twTable.rowCount()):
            item_text = self.twTable.item(i, 1).text()
            retval[self.twTable.item(i, 0).text()] = item_text
        return retval

    @pyqtSlot(str)
    def onItemChanged(self, item):
        if item.column() != 1:
            return
        text = item.text()
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(len(text) > 0)


class SceneLinkDialog(QDialog):
    def __init__(self, model, parent=None):
        super(SceneLinkDialog, self).__init__(parent=parent)
        uic.loadUi(op.join(op.dirname(__file__), 'scenelink.ui'), self)

        self._selectedScene = 0
        self._sceneTargets = self._get_scenelink_targets()
        for scene in self._sceneTargets:
            self.lwScenes.addItem(scene)

        flags = Qt.MatchExactly | Qt.MatchCaseSensitive
        self._select_item(model.target, flags)

        self._selectedTargetWin = model.target_window
        radioButtons = {
            SceneTargetWindow.MainWindow: self.mainRadio,
            SceneTargetWindow.Dialog: self.dialogRadio
        }
        button = radioButtons.get(self._selectedTargetWin)
        if button:
            button.setChecked(True)

    def _get_scenelink_targets(self):
        project = get_project_model().root_model
        if project is None:
            return []

        collected = set()

        def visitor(obj):
            nonlocal collected
            if isinstance(obj, SceneModel):
                target = "{}:{}".format(obj.simple_name, obj.uuid)
                collected.add(target)

        walk_traits_object(project, visitor)
        return sorted(collected)

    def _select_item(self, text, flags):
        items = self.lwScenes.findItems(text, flags)
        if items:
            # Select first result
            self.lwScenes.setCurrentItem(items[0])
        self._selectedScene = self.lwScenes.currentRow()

    @property
    def selectedScene(self):
        return self._sceneTargets[self._selectedScene]

    @property
    def selectedTargetWindow(self):
        return self._selectedTargetWin

    @pyqtSlot(int)
    def on_lwScenes_currentRowChanged(self, index):
        self._selectedScene = index

    @pyqtSlot(bool)
    def on_dialogRadio_clicked(self, checked=False):
        if checked:
            self._selectedTargetWin = SceneTargetWindow.Dialog

    @pyqtSlot(bool)
    def on_mainRadio_clicked(self, checked=False):
        if checked:
            self._selectedTargetWin = SceneTargetWindow.MainWindow

    @pyqtSlot(str)
    def on_leFilter_textChanged(self, text):
        self._select_item(text, Qt.MatchStartsWith)
