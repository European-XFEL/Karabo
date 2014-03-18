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


from widget import DisplayWidget
import copy

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DisplayImageElement(DisplayWidget):
    category = "ImageElement"
    alias = "Image Element"

    def __init__(self, box, parent):
        super(DisplayImageElement, self).__init__(box)
        
        self.__image = QLabel(parent)
        self.__image.setAutoFillBackground(True)
        self.__image.setAlignment(Qt.AlignCenter)
        self.__image.setMinimumHeight(125)
        self.__image.setMaximumHeight(125)
        self.__image.setMinimumWidth(125)
        self.__image.setWordWrap(True)
        self.setErrorState(False)
        self.value = None
        

    @property
    def widget(self):
        return self.__image


    def setErrorState(self, isError):
        if isError is True:
            self.__image.setStyleSheet("QLabel { background-color : rgba(255,155,155,128); }") # light red
        else:
            self.__image.setStyleSheet("QLabel { background-color : rgba(225,242,225,128); }") # light green


    def valueChanged(self, box, value, timestamp=None):
        if self.value is None or value is not self.value:
            # Value as Hash (dimX=<dimX>, dimY=<dimY>, dimZ=<dimZ>, dimC=<dimC>, data=<data>)

            dimX, dimY = value.dims.value
            data = value.data.value
            
            if (dimX < 1) or (dimY < 1) or (len(data) < (dimX*dimY)): return

            image = QImage(data, dimX, dimY, QImage.Format_ARGB32_Premultiplied)            
            pixmap = QPixmap.fromImage(image)
            
            # Scale pixmap
            if pixmap.height() > 125:
                pixmap = pixmap.scaledToHeight(125)
                
            self.__image.setPixmap(pixmap)
