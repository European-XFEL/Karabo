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


from widget import DisplayWidget
import copy

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


class DisplayImage(DisplayWidget):
    category = "Image"
    alias = "Image View"

    def __init__(self, **params):
        super(DisplayImage, self).__init__(**params)
        
        self.value = None
        
        if useGuiQwt:           
            self.__imageWidget = ImageDialog(edit=False, toolbar=True,
                wintitle=self.keys[0])
            self.__image = None
            self.__plot = self.__imageWidget.get_plot()
        else:
            self.__imageWidget = QLabel()

    
    @property
    def widget(self):
        return self.__imageWidget


    def valueChanged(self, key, value, timestamp=None):
        if value is None: return

        if self.value is None or value is not self.value:
            # Store original value with type
            self.value = copy.copy(value)
            
            if value.has('dims')==False: return
            if value.has('data')==False: return
            
            # Value as Hash (dimX=<dimX>, dimY=<dimY>, dimZ=<dimZ>, dimC=<dimC>, data=<data>)
            dims = value.get('dims')
            if len(dims) < 2: return
            dimX = dims[0]
            dimY = dims[1]
            #dimZ = dims[2]
            data = value.get('data')
            
            if not dimX and not dimY and not data: return
            if (dimX < 1) or (dimY < 1) or (len(data) < (dimX*dimY)): return

            image = QImage(data, dimX, dimY, QImage.Format_ARGB32_Premultiplied)
            
            if useGuiQwt:
                data = image.bits().asstring(image.numBytes())
                npy = np.frombuffer(data, np.uint8)
                npy.shape = image.height(), image.bytesPerLine()/4, 4
                if self.__image is None:
                    self.__image = make.image(npy)
                    self.__plot.add_item(self.__image)                    
                else:                    
                    self.__image.set_data(npy)
                    self.__plot.replot()
            else:
                pixmap = QPixmap.fromImage(image)
                self.__imageWidget.setPixmap(pixmap)
