#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a wrapper of a TaurusLabel."""

__all__ = ["TaurusLabelWrapper"]


from displaywidget import DisplayWidget
# not yet working under ubuntu 12.04
#from taurus.qt.qtgui.display import TaurusLabel


def getCategoryAliasClassName():
    return ["TaurusWidget","Taurus Label","TaurusLabelWrapper"]


class TaurusLabelWrapper(DisplayWidget):
  
    def __init__(self, **params):
        super(TaurusLabelWrapper, self).__init__(**params)
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        self.__label = TaurusLabel()
        
        self.__key = params.get('key')
        
        # Set value
        value = params.get('value')
        if value is not None:
            self.valueChanged(self.__key, value)

    def _getCategory(self):
        category, alias, className = getCategoryAliasClassName()
        return category
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__label
    widget = property(fget=_getWidget)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return [self.__key]
    keys = property(fget=_getKeys)


    def _value(self):
        return str("TaurusLabelWrapper")#str(self.__label.text())
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
            return TaurusLabelWrapper(**params)

