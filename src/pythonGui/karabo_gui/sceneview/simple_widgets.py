#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QByteArray
from PyQt4.QtGui import QFrame, QLabel, QPainter, QWidget
from PyQt4.QtSvg import QSvgRenderer

from karabo_gui.scenemodel.api import write_single_model


class LabelWidget(QLabel):
    """ A label which can appear in a scene
    """

    def __init__(self, model, parent=None):
        super(LabelWidget, self).__init__(model.text, parent)
        self.setFrameShape(QFrame.Box)
        self.setLineWidth(model.frame_width)
        self.setGeometry(model.x, model.y, model.width, model.height)

        styleSheet = []
        styleSheet.append('qproperty-font: "{}";'.format(model.font))
        styleSheet.append('color: "{}";'.format(model.foreground))
        if model.background:
            styleSheet.append('background-color: "{}";'.format(
                model.background))
        self.setStyleSheet("".join(styleSheet))


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
