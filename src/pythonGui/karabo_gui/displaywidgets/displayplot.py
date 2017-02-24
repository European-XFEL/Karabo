#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

__all__ = ["DisplayPlot"]

from guiqwt.plot import CurveDialog
from guiqwt.builder import make
from numpy import arange

from karabo.middlelayer import NumpyVector
from karabo_gui.widget import DisplayWidget


class DisplayPlot(DisplayWidget):
    category = NumpyVector
    alias = "Plot"
    colorList = ["red", "green", "blue", "gray", "violet", "orange",
                 "lightgreen", "black"]

    def __init__(self, box, parent):
        super(DisplayPlot, self).__init__(None)
        
        self.widget = CurveDialog(edit=False, toolbar=True,
                                  wintitle="Plot")
        self.plot = self.widget.get_plot()
        self.curves = { }
        self.addBox(box)


    @property
    def boxes(self):
        return list(self.curves.keys())


    value = None


    def addBox(self, box):
        curve = make.curve([0, 1], [0, 1], box.path[-1],
                           self.colorList[len(self.curves)])
        self.curves[box] = curve
        self.plot.add_item(curve)
        return True


    def valueChanged(self, box, value, timestamp=None):
        self.curves[box].set_data(arange(len(value)), value)
        self.plot.replot()
