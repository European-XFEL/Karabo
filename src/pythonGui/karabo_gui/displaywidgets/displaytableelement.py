#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on September 10, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import Qt

from karabo.middlelayer import VectorHash
from karabo_gui.editablewidgets.editabletableelement import (
    EditableTableElement)


class DisplayTableElement(EditableTableElement):
    category = VectorHash
    priority = 90
    alias = "Display Table Element"

    def __init__(self, box, parent):
        super(DisplayTableElement, self).__init__(box, parent,
                                                  role=Qt.DisplayRole)

    def copy(self, item):
        copyWidget = DisplayTableElement(item=item)
        copyWidget.tableModel.setHashList(self.tableModel.getHashList())

        return copyWidget
