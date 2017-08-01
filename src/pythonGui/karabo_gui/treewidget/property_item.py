#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 11, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import Qt, QSize
from PyQt4.QtGui import QAction, QMenu

from karabo_gui.displaywidgets.displaylabel import DisplayLabel
import karabo_gui.icons as icons
from karabo_gui.util import write_only_property
from .base_item import BaseTreeWidgetItem


class PropertyTreeWidgetItem(BaseTreeWidgetItem):

    def __init__(self, box, parent, parentItem=None):
        super(PropertyTreeWidgetItem, self).__init__(box, parent, parentItem)
        self.setData(0, Qt.SizeHintRole, QSize(200, 32))
        self.setIcon(0, icons.folder)

        self.create_display_widget(DisplayLabel, box)
        self.connect_signal(box.signalUpdateComponent,
                            self.onDisplayValueChanged)

    @write_only_property
    def displayText(self, text):
        self.setText(0, text)
        self.treeWidget().resizeColumnToContents(0)

    def setupContextMenu(self):
        # item specific menu
        # add actions from attributeWidget
        if self.editable_widget is None:
            return None

        menu = QMenu()
        text = "Reset to default"
        self.__acResetToDefault = QAction(icons.revert, text, None)
        self.__acResetToDefault.setStatusTip(text)
        self.__acResetToDefault.setToolTip(text)
        self.__acResetToDefault.setIconVisibleInMenu(True)
        self.__acResetToDefault.triggered.connect(self.onSetToDefault)

        menu.addAction(self.__acResetToDefault)
        menu.addSeparator()
        return menu

    def setReadOnly(self, readOnly):
        if self.editable_widget is not None:
            self.editable_widget.widget.setEnabled(not readOnly)
        super(PropertyTreeWidgetItem, self).setReadOnly(readOnly)

    def onSetToDefault(self):
        self.box.descriptor.setDefault(self.box)

    def onDisplayValueChanged(self, box, value):
        # Update tooltip dialog, if visible
        if self.popupWidget and self.popupWidget.isVisible():
            self.updateToolTipDialog()
