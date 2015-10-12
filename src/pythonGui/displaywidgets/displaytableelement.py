#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on September 10, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class EditableWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["DisplayTableElement"]


from editablewidgets.editabletableelement import *
from PyQt4.QtCore import *
from karabo.hash import Hash
from karabo.hashtypes import VectorHash


class DisplayTableElement(EditableTableElement):
    category = VectorHash
    priority = 90
    alias = "Display Table Element"

    def __init__(self, box, parent):
        super(DisplayTableElement, self).__init__(box, parent, role=Qt.DisplayRole)
        
      

    def setErrorState(self, state):
        """"""
        
    

    def copy(self, item):
        copyWidget = DisplayTableElement(item=item)

        copyWidget.tableModel.setHashList(self.tableModel.getHashList())

        return copyWidget
    
    