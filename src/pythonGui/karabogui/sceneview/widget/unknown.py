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
from qtpy.QtCore import QByteArray, QPoint, QRect, Qt
from qtpy.QtGui import QBrush, QPainter, QPen
from qtpy.QtSvg import QSvgRenderer
from qtpy.QtWidgets import QWidget

from karabo.common.scenemodel.api import write_single_model
from karabogui.widgets.hints import KaraboSceneWidget


class UnknownSvgWidget(QWidget):
    """ A widget which can display data from an UnknownXMLDataModel.
    """

    def __init__(self, renderer, parent=None):
        super().__init__(parent)
        self.renderer = renderer
        self.setGeometry(renderer.viewBox())
        self.setToolTip("External SVG Element")

    def minimumSizeHint(self):
        return self.renderer.defaultSize()

    def paintEvent(self, event):
        with QPainter(self) as painter:
            self.renderer.render(painter)

    def sizeHint(self):
        return self.minimumSizeHint()

    @classmethod
    def create(cls, model, parent=None):
        """Create an instance of this widget from a model object.

        Returns None if there would be nothing to display.
        """
        xml = write_single_model(model).encode("utf-8")
        ar = QByteArray.fromRawData(xml)
        renderer = QSvgRenderer(ar)

        if renderer.isValid() and not renderer.defaultSize().isNull():
            return cls(renderer, parent=parent)
        return None

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

    def set_geometry(self, rect):
        """Satisfy the informal widget interface."""

    def translate(self, offset):
        """Satisfy the informal widget interface."""


class UnknownWidget(KaraboSceneWidget, QWidget):
    """A widget which can display data from an UnknownWidgetDataModel or from a
    BaseWidgetObjectData subclass which is unsupported for some reason.
    """

    def __init__(self, model, parent=None):
        super().__init__(model=model, parent=parent)
        klassname = type(self.model).__name__
        if hasattr(self.model, "klass"):
            klassname = self.model.klass
        keys = ", ".join(self.model.keys)
        self.setToolTip(f"{keys}: --- Unsupported widget type: {klassname}")
        self.setGeometry(QRect(model.x, model.y, model.width, model.height))

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

    def update_alarm(self):
        """Satisfy the informal widget interface."""

    def update_global_access_level(self, level):
        """Satisfy the informal widget interface."""

    def set_geometry(self, rect):
        self.model.trait_set(x=rect.x(), y=rect.y(),
                             width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.trait_set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)

    def paintEvent(self, event):
        with QPainter(self) as painter:
            unsupported_text = "Unsupported"

            rect = self.rect()
            boundary = rect.adjusted(2, 2, -2, -2)
            painter.fillRect(rect, QBrush(Qt.lightGray, Qt.FDiagPattern))

            pen = QPen(Qt.lightGray)
            pen.setJoinStyle(Qt.MiterJoin)
            pen.setWidth(4)
            painter.setPen(pen)
            painter.drawRect(boundary)

            metrics = painter.fontMetrics()
            text_rect = metrics.boundingRect(unsupported_text)
            pos = rect.center() - QPoint(text_rect.width() // 2,
                                         -text_rect.height() // 2)
            painter.setPen(QPen())
            painter.drawText(pos, unsupported_text)
