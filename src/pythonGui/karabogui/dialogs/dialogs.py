#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created on March 19, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from qtpy import uic
from qtpy.QtCore import QDateTime, Slot, QPoint, QSize, Qt
from qtpy.QtGui import QPixmap, QPen, QPainter, QIcon
from qtpy.QtWidgets import (
    QColorDialog, QComboBox, QDialog, QDialogButtonBox, QFormLayout,
    QTableWidgetItem)

from karabo.common.api import walk_traits_object
from karabo.common.scenemodel.api import SceneModel, SceneTargetWindow
from karabogui.events import (
    broadcast_event, register_for_broadcasts, unregister_from_broadcasts,
    KaraboEvent)
from karabogui import icons
from karabogui.singletons.api import (
    get_manager, get_network, get_project_model)

from .utils import get_dialog_ui


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
        uic.loadUi(get_dialog_ui('about.ui'), self)
        self.setAttribute(Qt.WA_DeleteOnClose)

        image_path = op.join(op.dirname(icons.__file__), 'tunnel.png')
        tunnel_img = QPixmap(image_path)
        self.imgLabel.setPixmap(tunnel_img)

        # Pattern matchers for a specific key combos
        self._cheat_codes = {
            _PatternMatcher('chooch'):
                lambda: get_network().togglePerformanceMonitor(),
            _PatternMatcher('bigdata'):
                lambda: get_manager().toggleBigDataPerformanceMonitor()
        }

    def keyPressEvent(self, event):
        text = event.text()
        for matcher, trigger in self._cheat_codes.items():
            if matcher.check(text):
                trigger()
        return super(AboutDialog, self).keyPressEvent(event)


class PenDialog(QDialog):
    linecaps = {Qt.FlatCap: "butt", Qt.SquareCap: "square",
                Qt.RoundCap: "round"}
    linejoins = {Qt.SvgMiterJoin: "miter", Qt.MiterJoin: "miter",
                 Qt.BevelJoin: "bevel", Qt.RoundJoin: "round"}
    pen_styles = [Qt.SolidLine, Qt.DashLine, Qt.DotLine, Qt.DashDotLine,
                  Qt.DashDotDotLine]

    def __init__(self, pen, brush=None, parent=None):
        super(PenDialog, self).__init__(parent)
        uic.loadUi(get_dialog_ui('pendialog.ui'), self)
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
    def __init__(self, devices, parent=None):
        super(ReplaceDialog, self).__init__(parent)
        uic.loadUi(get_dialog_ui('replacedialog.ui'), self)

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

    @Slot(QTableWidgetItem)
    def onItemChanged(self, item):
        if item.column() != 1:
            return
        text = item.text()
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(len(text) > 0)


class SceneLinkDialog(QDialog):
    def __init__(self, model, parent=None):
        super(SceneLinkDialog, self).__init__(parent=parent)
        uic.loadUi(get_dialog_ui('scenelink.ui'), self)

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

    @Slot(int)
    def on_lwScenes_currentRowChanged(self, index):
        self._selectedScene = index

    @Slot(bool)
    def on_dialogRadio_clicked(self, checked=False):
        if checked:
            self._selectedTargetWin = SceneTargetWindow.Dialog

    @Slot(bool)
    def on_mainRadio_clicked(self, checked=False):
        if checked:
            self._selectedTargetWin = SceneTargetWindow.MainWindow

    @Slot(str)
    def on_leFilter_textChanged(self, text):
        self._select_item(text, Qt.MatchStartsWith)


class SceneItemDialog(QDialog):
    """A dialog to modify the layout bounding rect coordinates
    """

    def __init__(self, x=0, y=0, title='SceneItem', max_x=1024, max_y=768,
                 parent=None):
        super(SceneItemDialog, self).__init__(parent)
        filepath = get_dialog_ui('sceneitem_dialog.ui')
        uic.loadUi(filepath, self)
        self.setModal(False)
        # Fill the dialog with start values!
        self.ui_x.setValue(x)
        self.ui_x.setMaximum(max_x)
        self.ui_y.setValue(y)
        self.ui_y.setMaximum(max_y)
        self.setWindowTitle(title)
        if parent is not None:
            # place dialog accordingly!
            point = parent.rect().bottomRight()
            global_point = parent.mapToGlobal(point)
            self.move(global_point - QPoint(self.width(), 0))

    @property
    def x(self):
        return self.ui_x.value()

    @property
    def y(self):
        return self.ui_y.value()


ONE_WEEK = "One Week"
ONE_DAY = "One Day"
ONE_HOUR = "One Hour"
TEN_MINUTES = "Ten Minutes"


class ConfigurationFromPastDialog(QDialog):
    def __init__(self, instance_id, parent=None):
        super(ConfigurationFromPastDialog, self).__init__(parent)
        filepath = get_dialog_ui('conftime.ui')
        uic.loadUi(filepath, self)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.instance_id = instance_id
        self.setModal(False)
        self.setWindowFlags(self.windowFlags() | Qt.WindowCloseButtonHint)
        self.ui_timepoint.setDateTime(QDateTime.currentDateTime())
        self.ui_instance_id.setText(instance_id)
        self.ui_request.clicked.connect(self._request_configuration)
        self.ui_show_device.clicked.connect(self._show_device)
        self.event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network
        }
        self.ui_show_device.setIcon(icons.deviceInstance)
        register_for_broadcasts(self.event_map)

    def _event_network(self, data):
        if not data.get("status"):
            self.close()

    @Slot()
    def _show_device(self):
        broadcast_event(KaraboEvent.ShowDevice, {'deviceId': self.instance_id,
                                                 'showTopology': True})

    @Slot()
    def accept(self):
        """The dialog was accepted and we can request a configuration"""
        self._request_configuration()
        super(ConfigurationFromPastDialog, self).accept()

    def _request_configuration(self):
        # Karabo time points are in UTC
        time_point = self.ui_timepoint.dateTime().toUTC()
        # Explicitly specifiy ISODate!
        time = str(time_point.toString(Qt.ISODate))
        get_network().onGetConfigurationFromPast(self.instance_id, time=time)

    def done(self, result):
        """Stop listening for broadcast events"""
        unregister_from_broadcasts(self.event_map)
        super(ConfigurationFromPastDialog, self).done(result)

    def _get_time_information(self, selected_time_point):
        current_date_time = QDateTime.currentDateTime()
        if selected_time_point == ONE_WEEK:
            # One week
            time_point = current_date_time.addDays(-7)
        elif selected_time_point == ONE_DAY:
            # One day
            time_point = current_date_time.addDays(-1)
        elif selected_time_point == ONE_HOUR:
            # One hour
            time_point = current_date_time.addSecs(-3600)
        elif selected_time_point == TEN_MINUTES:
            # Ten minutes
            time_point = current_date_time.addSecs(-600)

        return time_point

    def _set_timepoint(self, selected_time_point):
        time = self._get_time_information(selected_time_point)
        self.ui_timepoint.setDateTime(time)

    @Slot()
    def on_ui_one_week_clicked(self):
        self._set_timepoint(ONE_WEEK)

    @Slot()
    def on_ui_one_day_clicked(self):
        self._set_timepoint(ONE_DAY)

    @Slot()
    def on_ui_one_hour_clicked(self):
        self._set_timepoint(ONE_HOUR)

    @Slot()
    def on_ui_ten_minutes_clicked(self):
        self._set_timepoint(TEN_MINUTES)


class ResizeSceneDialog(QDialog):
    def __init__(self, size=QSize(), parent=None):
        super(ResizeSceneDialog, self).__init__(parent)
        self.setModal(False)
        ui_path = get_dialog_ui('resizedialog.ui')
        uic.loadUi(ui_path, self)
        self.width_spinbox.setValue(size.width())
        self.height_spinbox.setValue(size.height())
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

    @property
    def scene_size(self):
        return QSize(self.width_spinbox.value(), self.height_spinbox.value())
