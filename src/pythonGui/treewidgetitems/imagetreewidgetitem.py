#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 8, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which inherits from a BaseTreeWidgetItem and represents
   an image item and its parameters.
"""

__all__ = ["ImageTreeWidgetItem"]


from basetreewidgetitem import BaseTreeWidgetItem
from displaycomponent import DisplayComponent
from karabo.karathon import *

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class ImageTreeWidgetItem(BaseTreeWidgetItem):
    
    def __init__(self, path, parent, parentItem=None):
        
        super(ImageTreeWidgetItem, self).__init__(path, parent, parentItem)
        
        self.setIcon(0, QIcon(":image"))
        
        self.displayComponent = DisplayComponent("Image Element", key=path)
        self.treeWidget().setItemWidget(self, 1, self.displayComponent.widget)
        self.treeWidget().resizeColumnToContents(1)
        
        # Initialize image data as Hash
        self.__imgData = Hash()
        self.__imgData.set("dimX", 0)
        self.__imgData.set("dimY", 0)
        self.__imgData.set("dimZ", 0)
        self.__imgData.set("dimC", 0)
        self.__imgData.set("data", [])


### getter & setter functions ###
    def _setEnabled(self, enabled):
        pass
    enabled = property(fset=_setEnabled)


    def _setText(self, text):
        self.setText(0, text)
    displayText = property(fset=_setText)


    def _getDimX(self):
        return self.__imgData.get('dimX')
    def _setDimX(self, dimX):
        self.__imgData.set('dimX', dimX)
    dimX = property(fget=_getDimX, fset=_setDimX)


    def _getDimY(self):
        return self.__imgData.get('dimY')
    def _setDimY(self, dimY):
        self.__imgData.set('dimY', dimY)
    dimY = property(fget=_getDimY, fset=_setDimY)


    def _getDimZ(self):
        return self.__imgData.get('dimZ')
    def _setDimZ(self, dimZ):
        self.__imgData.set('dimZ', dimZ)
    dimZ = property(fget=_getDimZ, fset=_setDimZ)


    def _getDimC(self):
        return self.__imgData.get('dimC')
    def _setDimC(self, dimC):
        self.__imgData.set('dimC', dimC)
    dimC = property(fget=_getDimC, fset=_setDimC)


    def _getdata(self):
        return self.__imgData.get('data')
    def _setdata(self, data):
        self.__imgData.set('data', data)
    data = property(fget=_setdata, fset=_setdata)


    def _getImageData(self):
        return self.__imgData
    imageData = property(fget=_getImageData)


### public functions ###
    def setReadOnly(self, readOnly):
        BaseTreeWidgetItem.setReadOnly(self, readOnly)

