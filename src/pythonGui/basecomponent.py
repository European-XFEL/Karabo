#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a base class for all inherited
   factory classes for creation of certain widgets and bundles main functionalities.
"""

__all__ = ["BaseComponent"]


from manager import Manager
#from messagebox import MessageBox

from registry import Loadable, ns_karabo, ns_svg

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class BaseComponent(Loadable, QObject):
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


    def addKeyValue(self, key, value):
        raise NotImplementedError, "BaseComponent.addKeyValue"


    def removeKey(self, key):
        raise NotImplementedError, "BaseComponent.removeKey"


    def destroy(self):
        raise NotImplementedError, "BaseComponent.destroy"


    def changeWidget(self, classAlias):
        raise NotImplementedError, "BaseComponent.changeWidget"


    @pyqtSlot(str, object)
    def onDisplayValueChanged(self, key, value):
        pass


    def attributes(self):
        """ returns a dict of attibutes for saving """
        d = { }
        d[ns_karabo + "class"] = self.__class__.__name__
        d[ns_karabo + "widgetFactory"] = self.widgetFactory.factory.__name__
        d[ns_karabo + "classAlias"] = self.classAlias
        if self.classAlias == "Command":
            d[ns_karabo + "commandText"] = self.widgetFactory.widget.text()
            d[ns_karabo + "commandEnabled"] = "{}".format(
                self.widgetFactory.widget.isEnabled())
            d[ns_karabo + "allowedStates"] = ",".join(
                self.widgetFactory.allowedStates)
            d[ns_karabo + "command"] = self.widgetFactory.command
        d[ns_karabo + "key"] = ",".join(self.keys)
        return d


    @classmethod
    def load(cls, elem, layout):
        ks = "classAlias", "key", "widgetFactory"
        if elem.get(ns_karabo + "classAlias") == "Command":
            ks += "command", "allowedStates", "commandText"
        d = {k: elem.get(ns_karabo + k) for k in ks}
        d["commandEnabled"] = elem.get(ns_karabo + "commandEnabled") == "True"
        keys = d['key'].split(",")
        d['key'] = keys[0]
        component = cls(**d)
        for k in keys[1:]:
            component.addKey(k)
        # Register as visible device
        online = Manager().newVisibleDevice(component.keys[0])
        if not online:
            # TODO:
            #print "offline"
            pass
        component.widget.setAttribute(Qt.WA_NoSystemBackground, True)
        return component


    #def _changeColor(self, color):
    #    self.widget.setAutoFillBackground(True)
    #    palette = self.widget.palette()
    #    palette.setColor(QPalette.Normal, QPalette.Base, color)
    #    self.widget.setPalette(palette)
    #changeColor = property(fset=_changeColor)

