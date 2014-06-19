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
import icons


class ImageTreeWidgetItem(BaseTreeWidgetItem):
    
    def __init__(self, box, parent, parentItem=None):
        
        super(ImageTreeWidgetItem, self).__init__(box, parent, parentItem)

        self.setIcon(0, icons.image)
        self.classAlias = "Image View"
        
        self.displayComponent = DisplayComponent("Image Element", box, self.treeWidget())
        self.treeWidget().setItemWidget(self, 1, self.displayComponent.widget)
        self.treeWidget().resizeColumnToContents(1)
             

    def _setText(self, text):
        self.setText(0, text)
    displayText = property(fset=_setText)

