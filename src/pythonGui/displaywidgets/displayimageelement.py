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


from displaywidget import DisplayWidget
import copy

from PyQt4.QtCore import *
from PyQt4.QtGui import *

def getCategoryAliasClassName():
    return ["ImageElement","Image Element","DisplayImageElement"]


class DisplayImageElement(DisplayWidget):
  
    def __init__(self, **params):
        super(DisplayImageElement, self).__init__(**params)
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        self.__value = None
        
        self.__image = QLabel()
        self.__image.setAutoFillBackground(True)
        self.__image.setAlignment(Qt.AlignCenter)
        self.__image.setMinimumHeight(125)
        self.__image.setMaximumHeight(125)
        self.__image.setMinimumWidth(125)
        self.__image.setWordWrap(True)
        self.setErrorState(False)
        
        self.__key = params.get(QString('key'))
        if self.__key is None:
            self.__key = params.get('key')
        
        # Set value
        value = params.get(QString('value'))
        if value is None:
            value = params.get('value')
        if value is not None:
            self.valueChanged(self.__key, value)


    def _getCategory(self):
        category, alias, className = getCategoryAliasClassName()
        return category
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__image
    widget = property(fget=_getWidget)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return [self.__key]
    keys = property(fget=_getKeys)


    def _value(self):
        return self.__value
    value = property(fget=_value)


    def setErrorState(self, isError):
        if isError is True:
            self.__image.setStyleSheet("QLabel { background-color : rgba(255,155,155,128); }") # light red
        else:
            self.__image.setStyleSheet("QLabel { background-color : rgba(225,242,225,128); }") # light green


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def valueChanged(self, key, value, timestamp=None):
        if value is None: return
        
        if self.__value is None or value is not self.__value:
            # Store original value with type
            self.__value = copy.copy(value)
            
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
            
    class Maker:
        def make(self, **params):
            return DisplayImageElement(**params)

