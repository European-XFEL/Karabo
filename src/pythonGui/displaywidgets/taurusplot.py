#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a wrapper of a TaurusPlot."""

__all__ = ["TaurusPlotWrapper"]


from widget import DisplayWidget

#from ..extern.taurus import TaurusCustomSettings
#from ..extern.taurus.qt.qtgui.plot import *
#from ..extern.taurusplot import TaurusPlot

# not yet working under ubuntu 12.04
#from taurus.qt.qtgui.plot import TaurusPlot


class TaurusPlotWrapper(DisplayWidget):
    category = "TaurusWidget"
    alias = "Taurus Plot"

    def __init__(self, **params):
        super(TaurusPlotWrapper, self).__init__(**params)
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        self.__plot = TaurusPlot()
        
        self.__key = params.get('key')


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__plot
    widget = property(fget=_getWidget)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return [self.__key]
    keys = property(fget=_getKeys)


    def _value(self):
        return str("TaurusPlotWrapper")
    value = property(fget=_value)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return
