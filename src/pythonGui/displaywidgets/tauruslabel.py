#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a wrapper of a TaurusLabel."""

__all__ = ["TaurusLabelWrapper"]


from widget import DisplayWidget
# not yet working under ubuntu 12.04
#from taurus.qt.qtgui.display import TaurusLabel


class TaurusLabelWrapper(DisplayWidget):
    category = "TaurusWidget"
    alias = "Taurus Label"

    def __init__(self, **params):
        super(TaurusLabelWrapper, self).__init__(**params)
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        self.__label = TaurusLabel()
        
    @property
    def widget(self):
        return self.__label

    @property
    def value(self):
        return "TaurusLabelWrapper"


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return
