from collections import namedtuple
from functools import partial
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import QPoint, QRect, QSize, Qt, pyqtSlot
from PyQt4.QtGui import (
    QAction, QColor, QDialog, QPainter, QPen, QPushButton, QStackedLayout,
    QWidget)
from traits.api import Instance

from karabo.common.scenemodel.api import (
    DeviceSceneLinkModel, SceneTargetWindow)
from karabo.common.enums import ONLINE_STATUSES
from karabogui.alarms.api import NORM_COLOR
from karabogui.binding.api import get_binding_value, VectorStringBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)
from karabogui.dialogs.textdialog import TextDialog
from karabogui.request import call_device_slot
from karabogui.sceneview.widget.label import LabelWidget
from karabogui.util import handle_scene_from_server


def _get_device_id(keys):
    if not isinstance(keys, list):
        return ''
    try:
        return keys[0].split('.', 1)[0]
    except IndexError:
        return ''


LinkModel = namedtuple('LinkModel', ['target', 'target_window'])


class LinkSymbol(QWidget):
    """ A clickable widget which opens a detached scene
    """
    def paintEvent(self, event):
        with QPainter(self) as painter:
            boundary = self.rect().adjusted(2, 2, -2, -2)
            pt = boundary.topLeft()
            rects = [QRect(pt, QSize(7, 7)),
                     QRect(pt + QPoint(11, 0), QSize(7, 7))]
            # draw the blue chain links
            pen = QPen()
            pen.setColor(QColor(*NORM_COLOR))
            pen.setWidth(3)
            painter.setPen(pen)
            painter.drawRects(rects)
            pen.setColor(Qt.lightGray)
            painter.setPen(pen)
            painter.drawLine(pt + QPoint(4, 4), pt + QPoint(15, 4))
            # label and frame need not be drawn


class LinkWidget(QWidget):
    """ A clickable widget which opens a detached scene
    """
    def __init__(self, model, parent=None):
        super(LinkWidget, self).__init__(parent)
        self.model = model

        self.layout = QStackedLayout(self)
        self.layout.setStackingMode(QStackedLayout.StackAll)

        self.button = QPushButton(self)
        self.button.clicked.connect(self._handle_click)
        self.button.setCursor(Qt.PointingHandCursor)
        self.button.setFocusPolicy(Qt.NoFocus)
        self._set_tooltip()
        self.button.setFlat(True)
        self.layout.addWidget(self.button)

        symbol = LinkSymbol(self)
        self.layout.addWidget(symbol)

        self.label = LabelWidget(model, self)
        self.label.setAttribute(Qt.WA_TransparentForMouseEvents)
        self.layout.addWidget(self.label)
        self.layout.setAlignment(self.label, Qt.AlignCenter)

        self.set_label_model(model)
        self.set_link_model(model)
        self.setGeometry(QRect(model.x, model.y, model.width, model.height))

    def set_link_model(self, model):
        self.model.target = model.target
        self.model.target_window = model.target_window
        self._set_tooltip()

    def set_label_model(self, model):
        self.model.text = model.text
        self.model.frame_width = model.frame_width
        self.model.font = model.font
        self.model.foreground = model.foreground
        if model.background:
            self.model.background = model.background
        self.label.set_model(model)
        self.label.setAlignment(Qt.AlignCenter)

    def _set_tooltip(self):
        tooltip = "{}|{}".format(
            _get_device_id(self.model.keys), self.model.target)
        self.button.setToolTip(tooltip)

    @pyqtSlot()
    def _handle_click(self):
        device_id = _get_device_id(self.model.keys)
        scene_name = self.model.target
        target_window = self.model.target_window
        handler = partial(handle_scene_from_server, device_id, scene_name,
                          None, target_window)
        call_device_slot(handler, device_id, 'requestScene',
                         name=scene_name)


@register_binding_controller(ui_name='Device Scene Link',
                             klassname='DisplayDeviceSceneLink',
                             can_show_nothing=False,
                             is_compatible=with_display_type('Scenes'),
                             binding_type=VectorStringBinding)
class DisplayDeviceSceneLink(BaseBindingController):
    # The scene model class for this controller
    model = Instance(DeviceSceneLinkModel, args=())
    _internal_widget = Instance(LinkWidget, allow_none=True)

    def create_widget(self, parent):
        self._internal_widget = LinkWidget(self.model, parent)
        select_action = QAction("Configure Link", self._internal_widget)
        select_action.triggered.connect(self._select_scene)
        self._internal_widget.addAction(select_action)

        text_action = QAction("Configure Text", self._internal_widget)
        text_action.triggered.connect(self._edit_text)
        self._internal_widget.addAction(text_action)
        return self._internal_widget

    def on_device_status_update(self, device_status):
        """When the device_status goes to a non online status this method will
        be called to disable the scenelink.

        :param device_status: DeviceStatus(Enum)"""
        enable_widget = (device_status in ONLINE_STATUSES)
        self._internal_widget.setEnabled(enable_widget)

    @pyqtSlot()
    def _select_scene(self):
        if get_binding_value(self.proxy) is None:
            return
        dialog = DeviceSceneLinkDialog(
            scene_list=self.proxy.value, model=self.model)
        if dialog.exec() == QDialog.Rejected:
            return
        self._internal_widget.set_link_model(dialog.link_model)

    @pyqtSlot()
    def _edit_text(self):
        if get_binding_value(self.proxy) is None:
            return
        dialog = TextDialog(self.model)
        if dialog.exec() == QDialog.Rejected:
            return
        self._internal_widget.set_label_model(dialog.label_model)

    def binding_update(self, proxy):
        if self.model.target == '':
            self.model.target = proxy.value[0]
            self.model.target_window = SceneTargetWindow.Dialog


class DeviceSceneLinkDialog(QDialog):
    def __init__(self, scene_list, model, parent=None):
        """A dialog to select the device scene link

        :param scene_list: the list of the scenes
        :param model: the DeviceSceneLink model
        :param parent: The parent of the dialog
        """
        super(DeviceSceneLinkDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'device_scenelink.ui')
        uic.loadUi(filepath, self)
        for scene in scene_list:
            self.cbScenes.addItem(scene)

        if model.target != '':
            index = self.cbScenes.findText(model.target)
            self.cbScenes.setCurrentIndex(index)

        radioButtons = {
            SceneTargetWindow.MainWindow: self.mainRadio,
            SceneTargetWindow.Dialog: self.dialogRadio
        }
        self._selectedTargetWin = model.target_window
        button = radioButtons.get(self._selectedTargetWin)
        if button:
            button.setChecked(True)

    @property
    def link_model(self):
        return LinkModel(target=self.cbScenes.currentText(),
                         target_window=self._selectedTargetWin)

    @pyqtSlot(bool)
    def on_dialogRadio_clicked(self, checked=False):
        if checked:
            self._selectedTargetWin = SceneTargetWindow.Dialog

    @pyqtSlot(bool)
    def on_mainRadio_clicked(self, checked=False):
        if checked:
            self._selectedTargetWin = SceneTargetWindow.MainWindow
