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

__all__ = ["DisplayImageElement"]

from schema import ImageNode
from widget import DisplayWidget

from karabo import hashtypes

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QColor, QImage, QLabel, QPixmap

import numpy as np

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
        self.setErrorState(False)
        self.value = None


    @property
    def widget(self):
        return self.image


    def setErrorState(self, isError):
        if isError is True:
            self.image.setStyleSheet("QLabel { background-color : rgba(255,155,155,128); }") # light red
        else:
            self.image.setStyleSheet("QLabel { background-color : rgba(225,242,225,128); }") # light green


    def valueChanged(self, box, value, timestamp=None):
        if self.value is not None or value is self.value:
            return
        
        if len(value.dims) == 2:
            # Shape
            dimX = value.dims[1]
            dimY = value.dims[0]

            # Format: Grayscale
            format = QImage.Format_Indexed8

        elif len(value.dims) == 3:
            # Shape
            dimX = value.dims[2]
            dimY = value.dims[1]
            dimZ = value.dims[0]                

            if dimZ == 3:
                # Format: RGB
                format = QImage.Format_RGB888

            else:
                return
        else:
            return

        # Data type information
        type = value.dataType
        try:
            type = hashtypes.Type.fromname[type].numpy
        except KeyError as e:
            e.message = 'Image element has improper type "{}"'.format(type)
            raise

        # Data itself
        data = value.data
        npy = np.frombuffer(data, type)

        # Normalize
        npy = npy - npy.min()
        npy *= (255.0 / npy.max())

        # Cast
        npy = npy.astype(np.uint8)

        if format == QImage.Format_Indexed8:
            try:
                npy.shape = dimY, dimX
            except ValueError as e:
                e.message = 'Image has improper shape ({}, {}) for size {}'. \
                    format(dimX, dimY, len(npy))
                raise

        elif format == QImage.Format_RGB888:
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

        if format == QImage.Format_Indexed8:
            image = QImage(npy.data, dimX, dimY, dimX, format)
            image.setColorTable(self.colorTable)
        elif format == QImage.Format_RGB888:
            image = QImage(npy.data, dimX, dimY, dimX*dimZ, format)
        else:
            return
        pixmap = QPixmap.fromImage(image)

        # Scale pixmap
        if pixmap.height() > 125:
            pixmap = pixmap.scaledToHeight(125)

        self.image.setPixmap(pixmap)
