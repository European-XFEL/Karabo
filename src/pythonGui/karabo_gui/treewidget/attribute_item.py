#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 14, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from karabo_gui.components import DisplayComponent
import karabo_gui.icons as icons
from .base_item import BaseTreeWidgetItem


class AttributeTreeWidgetItem(BaseTreeWidgetItem):
    def __init__(self, box, parent, parentItem=None):
        super(AttributeTreeWidgetItem, self).__init__(box, parent, parentItem)
        self.setIcon(0, icons.signal)
        self.displayComponent = DisplayComponent("DisplayLabel", self.box)
        self.treeWidget().setItemWidget(self, 1, self.displayComponent.widget)
        self.treeWidget().resizeColumnToContents(1)

    def setupContextMenu(self):
        # item specific menu
        # add actions from attributeWidget
        if self.editableComponent is None:
            return

    def _setText(self, text):
        self.setText(0, text)
        self.treeWidget().resizeColumnToContents(0)
    displayText = property(fset=_setText)
