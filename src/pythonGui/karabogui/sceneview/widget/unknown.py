#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtCore import QByteArray, QPoint, QRect, Qt
from PyQt5.QtGui import QBrush, QPainter, QPen
from PyQt5.QtSvg import QSvgRenderer
from PyQt5.QtWidgets import QWidget

from karabo.common.scenemodel.api import write_single_model


class PlaceholderWidget(QWidget):
    """A widget which indicates to the user that something is missing or
    unsupported.
    """
    def __init__(self, text, parent=None):
        super(PlaceholderWidget, self).__init__(parent)
        self._text = text

    def paintEvent(self, event):
        with QPainter(self) as painter:
            rect = self.rect()
            boundary = rect.adjusted(2, 2, -2, -2)

            painter.fillRect(rect, QBrush(Qt.lightGray, Qt.FDiagPattern))

            pen = QPen(Qt.lightGray)
            pen.setJoinStyle(Qt.MiterJoin)
            pen.setWidth(4)
            painter.setPen(pen)
            painter.drawRect(boundary)

            metrics = painter.fontMetrics()
            text_rect = metrics.boundingRect(self._text)
            pos = rect.center() - QPoint(text_rect.width() / 2,
                                         -text_rect.height() / 2)
            painter.setPen(QPen())
            painter.drawText(pos, self._text)


class UnknownSvgWidget(QWidget):
    """ A widget which can display data from an UnknownXMLDataModel.
    """
    def __init__(self, renderer, parent=None):
        super(UnknownSvgWidget, self).__init__(parent)
        self.renderer = renderer
        self.setGeometry(renderer.viewBox())

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
        xml = write_single_model(model).encode('utf-8')
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

    def set_visible(self, visible):
        """Satisfy the informal widget interface."""

    def update_global_access_level(self, level):
        """Satisfy the informal widget interface."""

    def set_geometry(self, rect):
        """Satisfy the informal widget interface."""

    def translate(self, offset):
        """Satisfy the informal widget interface."""


class UnknownWidget(PlaceholderWidget):
    """A widget which can display data from an UnknownWidgetDataModel or from a
    BaseWidgetObjectData subclass which is unsupported for some reason.
    """
    def __init__(self, model, parent=None):
        super(UnknownWidget, self).__init__('Unsupported', parent)
        self.model = model
        klassname = type(self.model).__name__
        if hasattr(self.model, 'klass'):
            klassname = self.model.klass
        self.setToolTip('Unsupported widget type: ' + klassname)
        self.setGeometry(QRect(model.x, model.y, model.width, model.height))

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
