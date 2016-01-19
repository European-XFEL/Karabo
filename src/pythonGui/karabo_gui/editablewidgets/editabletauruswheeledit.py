#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a wrapper of a TaurusWheelEdit."""

__all__ = ["TaurusWheelEditWrapper"]


from widget import EditableWidget
# not yet working under ubuntu 12.04
#from taurus.qt.qtgui.input import TaurusWheelEdit


class TaurusWheelEditWrapper(EditableWidget):
    category = "TaurusWidget"
    alias = "Taurus WheelEdit"

    def __init__(self, box, parent):
        super(TaurusWheelEditWrapper, self).__init__(box)
        self.__wheelEdit = TaurusWheelEdit(parent)
        
        #self.valueChanged(self.keys[0], value)


    @property
    def widget(self):
        return self.__wheelEdit


    @property
    def value(self):
        return "TaurusWheelEditWrapper"


    def valueChanged(self, box, value, timestamp=None, forceRefresh=False):
        if value is None:
            return
