#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QByteArray, QPoint, QRect, QSize, Qt
from PyQt4.QtGui import (QDialog, QFont, QFontMetrics, QFrame, QLabel,
                         QPainter, QPen, QPushButton, QWidget)
from PyQt4.QtSvg import QSvgRenderer

from karabo_gui.dialogs.textdialog import TextDialog
from karabo_gui.scenemodel.api import write_single_model


class LabelWidget(QLabel):
    """ A label which can appear in a scene
    """

    def __init__(self, model, parent=None):
        super(LabelWidget, self).__init__(model.text, parent)
        self.setFrameShape(QFrame.Box)
        self.set_model(model)

    def set_model(self, model):
        """ Set the new ``model`` and update the widget properties.
        """
        self.model = model

        self.setText(self.model.text)
        self.setLineWidth(model.frame_width)

        styleSheet = []
        styleSheet.append('qproperty-font: "{}";'.format(model.font))
        styleSheet.append('color: "{}";'.format(model.foreground))
        if model.background:
            styleSheet.append('background-color: "{}";'.format(
                model.background))
        self.setStyleSheet("".join(styleSheet))

        font = QFont()
        font.fromString(model.font)
        fm = QFontMetrics(font)
        CONTENT_MARGIN = 10
        model.width = fm.width(model.text) + CONTENT_MARGIN
        model.height = fm.height() + CONTENT_MARGIN
        self.setGeometry(model.x, model.y, model.width, model.height)

    def set_geometry(self, rect):
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)

    def edit(self):
        dialog = TextDialog(self.model)
        if dialog.exec() == QDialog.Rejected:
            return

        self.set_model(dialog.label_model)


class SceneLinkWidget(QPushButton):
    def __init__(self, model, parent=None):
        super(SceneLinkWidget, self).__init__(parent)
        self.model = model

        self.setCursor(Qt.PointingHandCursor)
        self.clicked.connect(self._handle_click)
        self.setGeometry(QRect(model.x, model.y, model.width, model.height))

    def _handle_click(self):
        if len(self.model.target) > 0:
            print("Open scene:", self.model.target)

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

    def set_geometry(self, rect):
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)


class UnknownSvgWidget(QWidget):
    """ A widget which can display data from an UnknownXMLModel.
    """
    def __init__(self, renderer, parent=None):
        super(UnknownSvgWidget, self).__init__(parent)
        self.renderer = renderer
        self.setGeometry(renderer.viewBox())

    def paintEvent(self, event):
        with QPainter(self) as painter:
            self.renderer.render(painter)

    @classmethod
    def create(cls, model, parent=None):
        """ Create an instance of this widget from a model object.

        Returns None if there would be nothing to display.
        """
        xml = write_single_model(model)
        ar = QByteArray.fromRawData(xml)
        renderer = QSvgRenderer(ar)

        if renderer.isValid() and not renderer.defaultSize().isNull():
            return cls(renderer, parent=parent)
        return None

    def set_geometry(self, rect):
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)
