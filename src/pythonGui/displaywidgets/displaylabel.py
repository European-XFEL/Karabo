#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 30, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class DisplayWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["DisplayLabel"]


from widget import DisplayWidget
from karabo.hash import Hash
from karabo import hashtypes
import decimal
import re

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QLabel

from numpy import ndarray


class DisplayLabel(DisplayWidget):
    category = hashtypes.String, hashtypes.Simple
    alias = "Value Field"
  
    def __init__(self, box, parent):
        super(DisplayLabel, self).__init__(box)
        
        self.value = None
        
        self.widget = QLabel(parent)
        self.widget.setAutoFillBackground(True)
        self.widget.setAlignment(Qt.AlignCenter)
        self.widget.setMinimumWidth(160)
        self.widget.setMinimumHeight(32)
        self.widget.setWordWrap(True)
        self.setErrorState(False)


    def setErrorState(self, isError):
        self.widget.setStyleSheet("QLabel {{ background-color : rgba({}); }}".
                                  format('255,155,155,128' if isError
                                         else '225,242,225,128'))


    def valueChanged(self, box, value, timestamp=None):
        if (not isinstance(box.descriptor, hashtypes.Type)
            or isinstance(box.descriptor,
                          (hashtypes.Hash, hashtypes.VectorHash))):
            return # only simple types can be shown here

        if value is None:
            return

        self.value = value

        if isinstance(value, str):
            self.widget.setText(value[:30])
            return
        elif isinstance(value, bytes):
            return

        try:
            format = dict(bin='b{:b}', oct='o{:o}', hex='0x{:X}'
                          )[box.descriptor.displayType[:3]]
        except (TypeError, KeyError):
            if isinstance(box.descriptor, (hashtypes.Float,
                                           hashtypes.VectorFloat)):
                format = "{:.6g}"
            elif isinstance(box.descriptor, (hashtypes.Double,
                                             hashtypes.VectorDouble)):
                format = "{:.10g}"
            else:
                format = "{}"


        if isinstance(value, ndarray):
            ret = str(value)
        elif isinstance(value, list):
            ret = '[' + ', '.join(format.format(v) for v in value[:4])
            if len(value) > 4:
                ret += ', ..]'
            else:
                ret += ']'
        else:
            ret = format.format(value)

        self.widget.setText(ret)
