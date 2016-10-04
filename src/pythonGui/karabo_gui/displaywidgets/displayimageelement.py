#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on August 9, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class DisplayWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

import numpy as np
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QColor, QImage, QLabel, QPixmap

from karabo_gui.const import ERROR_COLOR_ALPHA, OK_COLOR
from karabo_gui.images import get_image_data, get_dimensions_and_format
from karabo_gui.schema import ImageNode
from karabo_gui.util import generateObjectName
from karabo_gui.widget import DisplayWidget


class DisplayImageElement(DisplayWidget):
    category = ImageNode
    alias = "Image Element"

    colorTable = [QColor(i,i,i).rgb() for i in range(256)]

    def __init__(self, box, parent):
        super(DisplayImageElement, self).__init__(box)

        self.image = QLabel(parent)
        self.image.setAutoFillBackground(True)
        self.image.setAlignment(Qt.AlignCenter)
        self.image.setMinimumHeight(125)
        self.image.setMaximumHeight(125)
        self.image.setMinimumWidth(125)
        self.image.setWordWrap(True)

        objectName = generateObjectName(self)
        self._styleSheet = ("QLabel#{}".format(objectName) +
                            " {{ background-color : rgba{}; }}")
        self.image.setObjectName(objectName)
        self.setErrorState(False)
        self.value = None

    @property
    def widget(self):
        return self.image

    def setErrorState(self, isError):
        color = ERROR_COLOR_ALPHA if isError else OK_COLOR
        ss = self._styleSheet.format( color)
        self.image.setStyleSheet(ss)

    def valueChanged(self, box, value, timestamp=None):
        if self.value is not None or value is self.value:
            return

        dimX, dimY, dimZ, format = get_dimensions_and_format(value)
        if dimX is None or dimY is None or (dimZ is not None and dimZ != 3):
            return

        npy = get_image_data(value)
        if npy is None:
            return
        # Normalize
        npy = npy - npy.min()
        np.multiply(npy, 255.0 / npy.max())

        # Cast
        npy = npy.astype(np.uint8)
        if format is QImage.Format_Indexed8:
            try:
                npy.shape = dimY, dimX
            except ValueError as e:
                e.message = 'Image has improper shape ({}, {}) for size {}'. \
                    format(dimX, dimY, len(npy))
                raise
        elif format is QImage.Format_RGB888:
            try:
                npy.shape = dimY, dimX, dimZ
            except ValueError as e:
                e.message = 'Image has improper shape ({}, {}, {}) for size\
                    {}'.format(dimX, dimY, dimZ, len(npy))
                raise
        else:
            return

        # Safety
        if dimX < 1 or dimY < 1:
            raise RuntimeError('Image has less than two dimensions')

        if format is QImage.Format_Indexed8:
            image = QImage(npy.data, dimX, dimY, dimX, format)
            image.setColorTable(self.colorTable)
        elif format is QImage.Format_RGB888:
            image = QImage(npy.data, dimX, dimY, dimX*dimZ, format)
        else:
            return
        pixmap = QPixmap.fromImage(image)

        # Scale pixmap
        if pixmap.height() > 125:
            pixmap = pixmap.scaledToHeight(125)

        self.image.setPixmap(pixmap)
