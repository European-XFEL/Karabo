#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 8, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which inherits from a BaseTreeWidgetItem and represents
   an image item and its parameters.
"""

__all__ = ["ImageTreeWidgetItem"]


from basetreewidgetitem import BaseTreeWidgetItem
from components import DisplayComponent

from PyQt4.QtGui import QIcon

class ImageTreeWidgetItem(BaseTreeWidgetItem):
    
    def __init__(self, path, parent, parentItem=None):
        
        super(ImageTreeWidgetItem, self).__init__(path, parent, parentItem)
        
        self.setIcon(0, QIcon(":image"))
        self.classAlias = "Image View"
        
        self.displayComponent = DisplayComponent(
            "Image Element", path, self.treeWidget())
        self.treeWidget().setItemWidget(self, 1, self.displayComponent.widget)
        self.treeWidget().resizeColumnToContents(1)
             

    def _setText(self, text):
        self.setText(0, text)
    displayText = property(fset=_setText)


### public functions ###
    def setToolTipDialogVisible(self, show):
        pass

