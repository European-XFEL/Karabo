#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a base class for all inherited
   factory classes for creation of certain widgets and bundles main functionalities.
"""

__all__ = ["BaseComponent"]


#from manager import Manager
#from messagebox import MessageBox

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class BaseComponent(QObject):
    # signals
    signalValueChanged = pyqtSignal(str, object) # key, value


    def __init__(self, classAlias):
        super(BaseComponent, self).__init__()
        
        self.__classAlias = classAlias
        
        # Describes the category of the widget, widget can contain only values
        # of same category
        #self.__dataCategory = None
        # Minimum and maximum number of associated keys, 1 by default for each
        #self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        # States, if the widget is associated with an online device from the distributed system
        self.__isOnline = False
        # Gives the position of the widget in the global coordinate system
        self.__windowPosition = None


    def _getClassAlias(self):
        return self.__classAlias
    def _setClassAlias(self, classAlias):
        self.__classAlias = classAlias
    classAlias = property(fget=_getClassAlias, fset=_setClassAlias)


    def _getWidgetCategory(self):
        raise NotImplementedError, "BaseComponent._getWidgetCategory"
    widgetCategory = property(fget=_getWidgetCategory)


    def _getWidgetFactory(self):
        raise NotImplementedError, "BaseComponent._getWidgetFactory"
    widgetFactory = property(fget=_getWidgetFactory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        raise NotImplementedError, "BaseComponent._getWidget"
    widget = property(fget=_getWidget)


    def _getKeys(self):
        raise NotImplementedError, "BaseComponent._getKeys"
    keys = property(fget=_getKeys)


    def _getValue(self):
        raise NotImplementedError, "BaseComponent._getValue"
    def _setValue(self, value):
        raise NotImplementedError, "BaseComponent._setValue"
    value = property(fget=_getValue, fset=_setValue)


    # Triggered by DataNotifier signalUpdateComponent
    def onValueChanged(self, key, value):
        raise NotImplementedError, "BaseComponent.onValueChanged"


    def addKeyValue(self, key, value):
        raise NotImplementedError, "BaseComponent.addKeyValue"


    def removeKey(self, key):
        raise NotImplementedError, "BaseComponent.removeKey"


    def destroy(self):
        raise NotImplementedError, "BaseComponent.destroy"


    def changeWidget(self, classAlias):
        raise NotImplementedError, "BaseComponent.changeWidget"


    #def _changeColor(self, color):
    #    self.widget.setAutoFillBackground(True)
    #    palette = self.widget.palette()
    #    palette.setColor(QPalette.Normal, QPalette.Base, color)
    #    self.widget.setPalette(palette)
    #changeColor = property(fset=_changeColor)

