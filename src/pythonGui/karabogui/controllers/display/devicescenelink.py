from functools import partial

from PyQt5.QtCore import QPoint, QRect, QRectF, QSize, Qt, pyqtSlot
from PyQt5.QtGui import QColor, QPainter, QPen
from PyQt5.QtWidgets import QAction, QDialog, QPushButton, QSizePolicy
from traits.api import Instance, Undefined

from karabogui import messagebox
from karabo.common.enums import ONLINE_STATUSES
from karabo.common.scenemodel.api import (
    DeviceSceneLinkModel, SceneTargetWindow)
from karabogui.alarms.api import NORM_COLOR
from karabogui.binding.api import get_binding_value, VectorStringBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)
from karabogui.dialogs.device_scenelink_dialog import DeviceSceneLinkDialog
from karabogui.dialogs.textdialog import TextDialog
from karabogui.fonts import get_font_from_string, substitute_font
from karabogui.request import call_device_slot
from karabogui.singletons.api import get_topology
from karabogui.util import handle_scene_from_server


def _get_device_id(keys):
    if not isinstance(keys, list):
        return ''
    try:
        return keys[0].split('.', 1)[0]
    except IndexError:
        return ''


class LinkWidget(QPushButton):
    """ A clickable widget which opens a detached scene

    This is an internal widget of a controller. Geometry management is done
    on its container, so avoid applying any geometry in this class.
    """

    def __init__(self, model, parent=None):
        super(LinkWidget, self).__init__(parent)
        self.model = model
        # Check and substitute the font with the application fonts
        substitute_font(model)
        self.setFont(get_font_from_string(model.font))
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.clicked.connect(self._handle_click)
        self.setCursor(Qt.PointingHandCursor)
        self.setFocusPolicy(Qt.NoFocus)
        self._set_tooltip()
        self.setGeometry(QRect(model.x, model.y, model.width, model.height))
        self.set_link_model(model)

    def paintEvent(self, event):
        with QPainter(self) as painter:
            boundary = self.rect().adjusted(0, 0, -1, -1)
            pt = boundary.topLeft()
            rects = [QRectF(QRect(pt, QSize(7, 7))),
                     QRectF(QRect(pt + QPoint(11, 0), QSize(7, 7)))]
            painter.fillRect(boundary, QColor(self.model.background))
            # Draw the boundary
            pen = QPen()
            pen.setColor(Qt.black)
            pen.setWidth(self.model.frame_width)
            painter.setPen(pen)
            painter.drawRect(boundary)
            # Draw the chain on top left
            pen.setColor(QColor(*NORM_COLOR))
            pen.setWidth(3)
            painter.setPen(pen)
            painter.drawRects(rects)
            pen.setColor(Qt.lightGray)
            painter.setPen(pen)
            painter.drawLine(pt + QPoint(4, 4), pt + QPoint(15, 4))
            # Before painting the text, set the font
            painter.setFont(self.font())
            pen = QPen(QColor(self.model.foreground))
            painter.setPen(pen)
            painter.drawText(boundary, Qt.AlignCenter, self.model.text)

    def set_link_model(self, model):
        self.model.target = model.target
        self.model.target_window = model.target_window
        self._set_tooltip()

    def set_label_model(self, model):
        # Set all at once!
        self.model.trait_set(text=model.text, frame_width=model.frame_width,
                             font=model.font, background=model.background,
                             foreground=model.foreground)

        # Record the QFont on the widget
        self.setFont(get_font_from_string(model.font))
        self.update()

    def _set_tooltip(self):
        tooltip = "{}|{}".format(
            _get_device_id(self.model.keys), self.model.target)
        self.setToolTip(tooltip)

    @pyqtSlot()
    def _handle_click(self):
        device_id = _get_device_id(self.model.keys)
        device = get_topology().get_device(device_id)

        if device is not None and device.status not in ONLINE_STATUSES:
            messagebox.show_warning("Device is not online!", "Warning",
                                    parent=self)
            return

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

    def create_widget(self, parent=None):
        widget = LinkWidget(self.model, parent)
        select_action = QAction("Configure Link", widget)
        select_action.triggered.connect(self._select_scene)
        widget.addAction(select_action)

        text_action = QAction("Configure Text", widget)
        text_action.triggered.connect(self._edit_text)
        widget.addAction(text_action)
        return widget

    def _select_scene(self):
        if get_binding_value(self.proxy) is None:
            return
        dialog = DeviceSceneLinkDialog(
            scene_list=self.proxy.value, model=self.model, parent=self.widget)
        if dialog.exec_() == QDialog.Rejected:
            return
        self.widget.set_link_model(dialog.link_model)

    def _edit_text(self):
        if get_binding_value(self.proxy) is None:
            return
        dialog = TextDialog(self.model, parent=self.widget)
        if dialog.exec_() == QDialog.Rejected:
            return
        self.widget.set_label_model(dialog.label_model)

    def binding_update(self, proxy):
        if proxy.value is Undefined or not proxy.value:
            return

        if self.model.target == '':
            self.model.target = proxy.value[0]
            self.model.target_window = SceneTargetWindow.Dialog
