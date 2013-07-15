#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 8, 2012
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

__all__ = ["DisplayImage"]


from displaywidget import DisplayWidget

try:
    import numpy as np
    from guiqwt.plot import ImageDialog
    from guiqwt.builder import make
    useGuiQwt = True
except:
    print "Package guiqwt not installed."
    useGuiQwt = False

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["Image","Image View","DisplayImage"]


class DisplayImage(DisplayWidget):
  
    def __init__(self, **params):
        super(DisplayImage, self).__init__(**params)
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        self.__value = None
        
        if useGuiQwt:
            # Use guiqwt ImageDialog
            self.__image = ImageDialog(edit=False, toolbar=True, wintitle="")
                                       #options=dict(show_contrast=True))
                                       #options=dict(xlabel='Concentration', xunit='ppm'))
        else:
            self.__image = QLabel()

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


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return

        if value != self.__value:
            # Store original value with type
            self.__value = value
            
            # Value as Hash (dimX=<dimX>, dimY=<dimY>, dimZ=<dimZ>, dimC=<dimC>, data=<data>)
            dims = value.get('dims')
            if len(dims) != 2: return
            dimX = dims[0]
            dimY = dims[1]
            #dimZ = dims[2]
            data = value.get('data')
            encoding = value.get('encoding')
            
            if not dimX and not dimY and not data: return
            if (dimX < 1) or (dimY < 1) or (len(data) < (dimX*dimY)): return

            image = QImage(data, dimX, dimY, QImage.Format_ARGB32_Premultiplied).rgbSwapped()
            
            if useGuiQwt:
                data = image.bits().asstring(image.numBytes())
                npy = np.frombuffer(data, np.uint8)
                npy.shape = image.height(), image.bytesPerLine()/4, 4
                imgItem = make.image(npy)
                plot = self.__image.get_plot()
                plot.add_item(imgItem)
            else:
                pixmap = QPixmap.fromImage(image)
                self.__image.setPixmap(pixmap)


    class Maker:
        def make(self, **params):
            return DisplayImage(**params)

