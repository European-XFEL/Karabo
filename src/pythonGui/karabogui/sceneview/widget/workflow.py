#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QRect, Qt
from PyQt4.QtGui import QColor, QFont, QFontMetrics, QPainter, QPen, QWidget

LIGHT_BLUE = (224, 240, 255)
PADDING = 10


class WorkflowItemWidget(QWidget):
    """A workflow item which can appear in a scene
    """
    def __init__(self, model, parent=None):
        super(WorkflowItemWidget, self).__init__(parent)
        self.model = model
        self.font = QFont()
        self.font.fromString(model.font)
        self.pen = QPen()
        if self.model.klass == 'WorkflowGroupItem':
            self.pen.setWidth(3)

        self.setToolTip(self.model.device_id)
        rect = QRect(model.x, model.y, model.width, model.height)
        self.setGeometry(rect)
        self.outline_rect = self._compute_outline(rect)
        self._minimum_rect = self._compute_minimum_rect()

    def minimumSize(self):
        return self._minimum_rect.size()

    def paintEvent(self, event):
        with QPainter(self) as painter:
            painter.setRenderHint(QPainter.Antialiasing)
            painter.setFont(self.font)
            painter.setPen(self.pen)

            painter.setBrush(QColor(*LIGHT_BLUE))
            painter.drawRoundRect(self.outline_rect, 5, 5)
            painter.drawText(self.outline_rect, Qt.AlignCenter,
                             self.model.device_id)

    def sizeHint(self):
        return self.minimumSize()

    def add_proxies(self, proxies):
        """Satisfy the informal widget interface."""

    def apply_changes(self, devices=None, send_immediately=False):
        """Satisfy the informal widget interface."""

    def decline_changes(self):
        """Satisfy the informal widget interface."""

    def destroy(self):
        """Satisfy the informal widget interface."""

    def set_visible(self, visible):
        """Satisfy the informal widget interface."""

    def update_alarm_symbol(self):
        """Satisfy the informal widget interface."""

    def update_global_access_level(self):
        """Satisfy the informal widget interface."""

    def set_geometry(self, rect):
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        self.setGeometry(rect)
        self.outline_rect = self._compute_outline(rect)

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)

    def _compute_outline(self, rect):
        padding = self.pen.width()
        rect.moveTo(0, 0)
        rect.adjust(padding, padding, -padding, -padding)
        return rect

    def _compute_minimum_rect(self):
        fm = QFontMetrics(self.font)
        rect = fm.boundingRect(self.model.device_id)
        rect.adjust(-PADDING, -PADDING, PADDING, PADDING)
        return rect
