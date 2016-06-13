#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QByteArray
from PyQt4.QtGui import QFont, QFontMetrics, QFrame, QLabel, QPainter, QWidget
from PyQt4.QtSvg import QSvgRenderer

from karabo_gui.scenemodel.api import write_single_model


class LabelWidget(QLabel):
    """ A label which can appear in a scene
    """

    def __init__(self, model, parent=None):
        super(LabelWidget, self).__init__(model.text, parent)
        self.model = model

        self.setFrameShape(QFrame.Box)
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
        if model.width == 0:
            model.width = fm.width(model.text) + CONTENT_MARGIN
        if model.height == 0:
            model.height = fm.height() + CONTENT_MARGIN
        self.setGeometry(model.x, model.y, model.width, model.height)

    def set_geometry(self, rect):
        self.setGeometry(rect)

    def translate(self, offset):
        self.move(offset)


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
        self.setGeometry(rect)

    def translate(self, offset):
        self.move(self.pos() + offset)
