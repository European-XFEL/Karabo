#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a wrapper of a TaurusWheelEdit."""

__all__ = ["TaurusWheelEditWrapper"]


from editablewidget import EditableWidget
# not yet working under ubuntu 12.04
#from taurus.qt.qtgui.input import TaurusWheelEdit


def getCategoryAliasClassName():
    return ["TaurusWidget","Taurus WheelEdit","TaurusWheelEditWrapper"]


class TaurusWheelEditWrapper(EditableWidget):
  
    def __init__(self, **params):
        super(TaurusWheelEditWrapper, self).__init__(**params)
        
        self.__wheelEdit = TaurusWheelEdit()
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        # Set key
        self.__key = params.get(QString('key'))
        if self.__key is None:
            self.__key = params.get('key')
        # Set value
        value = params.get(QString('value'))
        if value is None:
            value = params.get('value')
        self.valueChanged(self.__key, value)


    def _getCategory(self):
        category, alias, className = getCategoryAliasClassName()
        return category
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__wheelEdit
    widget = property(fget=_getWidget)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return [self.__key]
    keys = property(fget=_getKeys)


    def addParameters(self, **params):
        print "addParameters", params


    def _value(self):
        return str("TaurusWheelEditWrapper")
    value = property(fget=_value)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return


    class Maker:
        def make(self, **params):
            return TaurusWheelEditWrapper(**params)

