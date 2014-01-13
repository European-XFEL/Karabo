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

    def __init__(self, **params):
        super(DisplayImageElement, self).__init__(**params)
        
        self.value = None
        
        self.__image = QLabel()
        self.__image.setAutoFillBackground(True)
        self.__image.setAlignment(Qt.AlignCenter)
        self.__image.setMinimumHeight(125)
        self.__image.setMaximumHeight(125)
        self.__image.setMinimumWidth(125)
        self.__image.setWordWrap(True)
        self.setErrorState(False)
        

    @property
    def widget(self):
        return self.__image


    def setErrorState(self, isError):
        if isError is True:
            self.__image.setStyleSheet("QLabel { background-color : rgba(255,155,155,128); }") # light red
        else:
            self.__image.setStyleSheet("QLabel { background-color : rgba(225,242,225,128); }") # light green


    def valueChanged(self, key, value, timestamp=None):
        if value is None: return
        
        if self.value is None or value is not self.value:
            # Store original value with type
            self.value = copy.copy(value)
            
            if value.has('dims')==False: return
            if value.has('data')==False: return

            # Value as Hash (dimX=<dimX>, dimY=<dimY>, dimZ=<dimZ>, dimC=<dimC>, data=<data>)
            dims = value.get('dims')
            if len(dims) < 2: return;
            dimX = dims[0]
            dimY = dims[1]
            #dimZ = dims[2]
            data = value.get('data')
            
            if not dimX and not dimY and not data: return
            if (dimX < 1) or (dimY < 1) or (len(data) < (dimX*dimY)): return

            image = QImage(data, dimX, dimY, QImage.Format_ARGB32_Premultiplied)            
            pixmap = QPixmap.fromImage(image)
            
            # Scale pixmap
            if pixmap.height() > 125:
                pixmap = pixmap.scaledToHeight(125)
                
            self.__image.setPixmap(pixmap)
