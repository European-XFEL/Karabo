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

    def __init__(self, box, parent):
        super(TaurusPlotWrapper, self).__init__(box)
        
        self.__plot = TaurusPlot(parent)


    @property
    def widget(self):
        return self.__plot


    @property
    def value(self):
        return "TaurusPlotWrapper"

    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            return
