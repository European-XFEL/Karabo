#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 8, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from karabo_gui.components import DisplayComponent
import karabo_gui.icons as icons
from .base_item import BaseTreeWidgetItem


class ImageTreeWidgetItem(BaseTreeWidgetItem):
    def __init__(self, box, parent, parentItem=None):
        super(ImageTreeWidgetItem, self).__init__(box, parent, parentItem)

        self.setIcon(0, icons.image)
        self.displayComponent = DisplayComponent("DisplayImageElement", box,
                                                 self.treeWidget())
        self.treeWidget().setItemWidget(self, 1, self.displayComponent.widget)
        self.treeWidget().resizeColumnToContents(1)

    def _setText(self, text):
        self.setText(0, text)
    displayText = property(fset=_setText)
