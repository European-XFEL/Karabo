#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
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
import webbrowser

from qtpy.QtCore import QPoint, QRect, QRectF, QSize, Qt, Slot
from qtpy.QtGui import QColor, QPainter, QPen
from qtpy.QtWidgets import (
    QAction, QDialog, QLabel, QPushButton, QStackedLayout)

from karabo.common.scenemodel.api import SceneTargetWindow
from karabogui import messagebox
from karabogui.binding.api import ProxyStatus
from karabogui.dialogs.api import (
    DeviceCapabilityDialog, SceneLinkDialog, TextDialog, WebDialog)
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.fonts import get_qfont
from karabogui.indicators import get_device_status_pixmap
from karabogui.request import get_scene_from_server
from karabogui.singletons.api import get_topology
from karabogui.topology.api import is_device_online
from karabogui.widgets.hints import KaraboSceneWidget


class BaseLinkWidget(KaraboSceneWidget, QPushButton):
    """The base link widget for the scene"""

    iconColor = None

    def __init__(self, model, parent=None):
        super().__init__(model=model, parent=parent)
        self.setFont(get_qfont(model.font))
        self.setToolTip(self.getToolTip())
        self.setCursor(Qt.PointingHandCursor)
        self.clicked.connect(self.handle_click)
        self.setGeometry(QRect(model.x, model.y, model.width, model.height))

        edit_action = QAction("Edit Label", self)
        edit_action.triggered.connect(self.edit_label)
        self.addAction(edit_action)

        target_action = QAction("Edit Target", self)
        target_action.triggered.connect(self.edit_target)
        self.addAction(target_action)

    def set_geometry(self, rect):
        """Satisfy the informal widget interface."""
        self.model.trait_set(x=rect.x(), y=rect.y(),
                             width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        """Satisfy the informal widget interface."""
        new_pos = self.pos() + offset
        self.model.trait_set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)

    def add_proxies(self, proxies):
        """Satisfy the informal widget interface."""

    def apply_changes(self):
        """Satisfy the informal widget interface."""

    def decline_changes(self):
        """Satisfy the informal widget interface."""

    def destroy(self):
        """Satisfy the informal widget interface."""
        super().destroy()

    def set_visible(self, visible):
        """Satisfy the informal widget interface."""

    def update_global_access_level(self, level):
        """Satisfy the informal widget interface."""

    def paintEvent(self, event):
        with QPainter(self) as painter:
            boundary = self.rect().adjusted(0, 0, -1, -1)
            pt = boundary.topLeft()
            rects = [QRectF(QRect((pt), QSize(7, 7))),
                     QRectF(QRect((pt) + QPoint(11, 0), QSize(7, 7)))]
            painter.fillRect(boundary, QColor(self.model.background))
            pen = QPen(Qt.black)
            pen.setWidth(self.model.frame_width)
            painter.setPen(pen)
            painter.drawRect(boundary)
            pen.setColor(self.iconColor)
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

    def edit(self, scene_view=None):
        """Double click handle for the scene view"""
        self.on_edit()
        self.setToolTip(self.getToolTip())

    # Qt Slots
    # -----------------------------------------------------------------------

    @Slot()
    def edit_label(self):
        dialog = TextDialog(self.model, parent=self)
        if dialog.exec() == QDialog.Rejected:
            return

        # Set all at once!
        label = dialog.label_model
        self.model.trait_set(text=label.text, frame_width=label.frame_width,
                             font=label.font, background=label.background,
                             foreground=label.foreground)

        # Record the QFont on the widget
        self.setFont(get_qfont(label.font))
        self.update()

    @Slot()
    def edit_target(self):
        self.on_edit()

    @Slot()
    def handle_click(self):
        self.on_click()

    # Public abstract interface
    # -----------------------------------------------------------------------

    def getToolTip(self):
        return self.model.target

    def on_edit(self):
        """Subclass this method to provide an action on `edit`"""

    def on_click(self):
        """Subclass this method to provide an action on `click`"""


class SceneLinkWidget(BaseLinkWidget):
    """A clickable link which opens another `scene`"""
    iconColor = Qt.darkGray

    def on_click(self):
        """Reimplemented function of `BaseLinkWidget`"""
        if len(self.model.target) > 0:
            parts = self.model.target.split(":")
            if len(parts) != 2:
                return

            # target format => "simple_name:UUID"
            name = parts[0]
            target = parts[1]
            target_window = self.model.target_window
            # Broadcast an event
            data = {"name": name, "target": target,
                    "target_window": target_window}
            broadcast_event(KaraboEvent.OpenSceneLink, data)

    def on_edit(self):
        """Reimplemented function of `BaseLinkWidget"""
        dialog = SceneLinkDialog(self.model, parent=self)
        if dialog.exec() == QDialog.Rejected:
            return

        self.model.target = dialog.selectedScene
        self.model.target_window = dialog.selectedTargetWindow


class WebLinkWidget(BaseLinkWidget):
    """A clickable widget which opens a hyperlink
    """
    iconColor = QColor(255, 145, 255)

    def on_click(self):
        """Reimplemented function of `BaseLinkWidget`"""
        if len(self.model.target) > 0:
            try:
                webbrowser.open_new(self.model.target)
            except webbrowser.Error:
                messagebox.show_error("No web browser available!", parent=self)

    def on_edit(self):
        """Reimplemented function of `BaseLinkWidget`"""
        dialog = WebDialog(self.model.target, parent=self)
        if dialog.exec() == QDialog.Rejected:
            return
        self.model.target = dialog.target


def _get_device_id(keys):
    if not isinstance(keys, list):
        return ''
    try:
        return keys[0].split('.', 1)[0]
    except IndexError:
        return ''


class DeviceSceneLinkWidget(BaseLinkWidget):
    """A clickable widget which opens a Device Scene Link
    """
    iconColor = QColor(100, 149, 237)

    def __init__(self, model, parent=None):
        super().__init__(model, parent)
        self.status_symbol = QLabel("", self)
        self.status_symbol.setAttribute(Qt.WA_TransparentForMouseEvents)
        pixmap = get_device_status_pixmap(ProxyStatus.OFFLINE)
        self.status_symbol.setPixmap(pixmap)

        self.layout = QStackedLayout(self)
        self.layout.setStackingMode(QStackedLayout.StackAll)
        self.layout.addWidget(self.status_symbol)

        self.proxy = get_topology().get_device(self.deviceId, False)
        self.proxy_status_change(self.proxy.status)
        self.proxy.on_trait_change(self.proxy_status_change, "status")

    @property
    def deviceId(self):
        return _get_device_id(self.model.keys)

    def proxy_status_change(self, status):
        if status is ProxyStatus.OFFLINE:
            self.status_symbol.show()
        else:
            self.status_symbol.hide()

    def getToolTip(self):
        """Reimplemented function of `BaseLinkWidget`"""
        return f"{self.deviceId}|{self.model.target}"

    def destroy(self):
        """Reimplemented function of `BaseLinkWidget`"""
        self.proxy.on_trait_change(self.proxy_status_change, "status",
                                   remove=True)
        self.proxy = None
        super().destroy()

    def on_click(self):
        """Reimplemented function of `BaseLinkWidget`"""
        device_id = self.deviceId
        if not is_device_online(device_id):
            messagebox.show_warning(
                f"Device <b>{device_id}</b> is not online!",
                "Warning", parent=self)
            return

        scene_name = self.model.target
        target_window = self.model.target_window
        get_scene_from_server(device_id, scene_name=scene_name,
                              target_window=target_window)

    def on_edit(self):
        """Reimplemented function of `BaseLinkWidget"""
        dialog = DeviceCapabilityDialog(self.deviceId, parent=self)
        dialog.find_and_select(self.model.target)
        if dialog.exec() == QDialog.Rejected:
            return
        self.model.target = dialog.capa_name
        self.model.target_window = SceneTargetWindow.Dialog
