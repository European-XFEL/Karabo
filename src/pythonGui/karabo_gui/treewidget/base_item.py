#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on April 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import OrderedDict

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QCursor, QTreeWidgetItem

from karabo.middlelayer import Type
from karabo_gui.alarms.api import ALARM_LOW, ALARM_HIGH, WARN_LOW, WARN_HIGH
from karabo_gui.popupwidget import PopupWidget


class BaseTreeWidgetItem(QTreeWidgetItem):
    isChoiceElement = False
    isListElement = False
    description = None

    def __init__(self, box, parent, parentItem=None):
        if parentItem:
            super(BaseTreeWidgetItem, self).__init__(parentItem)
        else:
            super(BaseTreeWidgetItem, self).__init__(parent)

        self.box = box

        self.__editableComponent = None
        self._contextMenu = None

        # Popup widget for tooltip info
        self.popupWidget = None

    def create_display_widget(self, klass, box):
        tree_widget = self.treeWidget()
        # Create a display widget
        self.display_widget = klass(box, tree_widget)
        box.signalUpdateComponent.connect(self.display_widget.valueChangedSlot)
        if box.hasValue():
            self.display_widget.valueChanged(box, box.value, box.timestamp)

        self.display_widget.setReadOnly(True)
        tree_widget.setItemWidget(self, 1, self.display_widget.widget)
        tree_widget.resizeColumnToContents(1)

    @property
    def editableComponent(self):
        """Returns the editable component of the item
        """
        return self.__editableComponent

    @editableComponent.setter
    def editableComponent(self, component):
        if component is None:
            return

        self.__editableComponent = component

        self._contextMenu = self.setupContextMenu()
        self.treeWidget().setItemWidget(self, 2, component.widget)
        self.treeWidget().resizeColumnToContents(2)

    def destroy(self):
        """Give item subclasses a chance to clean up signal connections"""

    def setupContextMenu(self):
        raise NotImplementedError("BaseTreeWidgetItem.setupContextMenu")

    def showContextMenu(self):
        if self._contextMenu is None:
            return
        self._contextMenu.exec_(QCursor.pos())

    def setErrorState(self, isError):
        if self.display_widget:
            self.display_widget.setErrorState(isError)

    def setReadOnly(self, readOnly):
        if readOnly is True:
            self.setFlags(self.flags() & ~Qt.ItemIsEnabled)
        else:
            self.setFlags(self.flags() | Qt.ItemIsEnabled)

    def setToolTipDialogVisible(self, show):
        if not self.popupWidget:
            self.popupWidget = PopupWidget(self.treeWidget())

        if show:
            self.updateToolTipDialog()
            pos = QCursor.pos()
            pos.setX(pos.x() + 10)
            pos.setY(pos.y() + 10)
            self.popupWidget.move(pos)
            self.popupWidget.show()
        else:
            self.popupWidget.hide()

    def updateToolTipDialog(self):
            info = OrderedDict()

            if len(self.text(0)) > 0:
                info["Property"] = self.text(0)
            d = self.box.descriptor
            if d.description is not None:
                info["Description"] = d.description

            info["Key"] = '.'.join(self.box.path)
            if isinstance(d, Type):
                info["Value Type"] = d.hashname()
            if d.defaultValue is not None:
                info["Default Value"] = d.defaultValue
            if d.alias is not None:
                info["Alias"] = d.alias
            if d.tags is not None:
                info["Tags"] = ", ".join(d.tags)
            if self.box.timestamp is not None:
                info["Timestamp"] = self.box.timestamp.toLocal()
            if d.displayType and d.displayType.startswith('bin|'):
                info["Bits"] = d.displayType[4:]
            if self.box.configuration.type == "device":
                info["Value on device"] = self.box.value

            alarms = [('Warn low', WARN_LOW), ('Warn high', WARN_HIGH),
                      ('Alarm low', ALARM_LOW), ('Alarm high', ALARM_HIGH)]
            for label, alarm in alarms:
                this_alarm = getattr(d, alarm)
                info[label] = 'n/a' if this_alarm is None else this_alarm

            self.popupWidget.setInfo(info)
