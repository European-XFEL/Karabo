#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import webbrowser

from qtpy.QtCore import QPoint, QRect, QRectF, QSize, Qt, Slot
from qtpy.QtGui import QColor, QPainter, QPen
from qtpy.QtWidgets import QAction, QDialog, QPushButton

from karabogui import messagebox
from karabogui.dialogs.api import SceneLinkDialog, TextDialog, WebDialog
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.fonts import get_qfont
from karabogui.widgets.hints import KaraboSceneWidget


class BaseLinkWidget(KaraboSceneWidget, QPushButton):
    """The base link widget for the scene"""

    iconColor = None

    def __init__(self, model, parent=None):
        super().__init__(model=model, parent=parent)
        self.setFont(get_qfont(model.font))
        self.setToolTip(self.model.target)
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
        self.setToolTip(self.model.target)


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
        self.setToolTip(self.model.target)
