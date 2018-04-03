#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QPoint, QRect, QSize, Qt
from PyQt4.QtGui import QDialog, QPainter, QPen, QPushButton

from karabogui.dialogs.dialogs import SceneLinkDialog
from karabogui.events import broadcast_event, KaraboEventSender


class SceneLinkWidget(QPushButton):
    """A clickable link which opens another scene
    """
    def __init__(self, model, parent=None):
        super(SceneLinkWidget, self).__init__(parent)
        self.model = model

        self.setToolTip(self.model.target)
        self.setCursor(Qt.PointingHandCursor)
        self.clicked.connect(self._handle_click)
        self.setGeometry(QRect(model.x, model.y, model.width, model.height))

    def paintEvent(self, event):
        with QPainter(self) as painter:
            boundary = self.rect().adjusted(2, 2, -2, -2)
            pt = boundary.topLeft()
            rects = [QRect(pt, QSize(7, 7)),
                     QRect(pt + QPoint(11, 0), QSize(7, 7))]

            pen = QPen(Qt.black)
            painter.drawRect(boundary)
            pen.setColor(Qt.darkGray)
            pen.setWidth(3)
            painter.setPen(pen)
            painter.drawRects(rects)
            pen.setColor(Qt.lightGray)
            painter.setPen(pen)
            painter.drawLine(pt + QPoint(4, 4), pt + QPoint(15, 4))

    def _handle_click(self):
        if len(self.model.target) > 0:
            parts = self.model.target.split(':')
            if len(parts) != 2:
                return

            # target format => "simple_name:UUID"
            target = parts[1]
            target_window = self.model.target_window
            # Broadcast an event
            data = {'target': target, 'target_window': target_window}
            broadcast_event(KaraboEventSender.OpenSceneLink, data)

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

    def update_alarm(self):
        """Satisfy the informal widget interface."""

    def update_global_access_level(self, level):
        """Satisfy the informal widget interface."""

    def set_geometry(self, rect):
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)

    def edit(self, scene_view):
        dialog = SceneLinkDialog(self.model, parent=scene_view)
        if dialog.exec() == QDialog.Rejected:
            return

        self.model.target = dialog.selectedScene
        self.model.target_window = dialog.selectedTargetWindow
