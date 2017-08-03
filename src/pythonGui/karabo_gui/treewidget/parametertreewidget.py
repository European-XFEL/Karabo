#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import json

from PyQt4.QtCore import pyqtSignal, QMimeData, QPoint, QRect, Qt
from PyQt4.QtGui import (QAbstractItemView, QCursor, QHeaderView, QMenu,
                         QTreeWidget)

import karabo_gui.globals as krb_globals
from karabo_gui.schema import ChoiceOfNodes
from karabo_gui.singletons.api import get_network, get_topology
from karabo_gui.widget import DisplayWidget, EditableWidget, Widget
from .command_item import CommandTreeWidgetItem
from .property_item import PropertyTreeWidgetItem


def getDeviceBox(box):
    """Return a box that belongs to an active device

    if the box already is part of a running device, return it,
    if it is from a class in a project, return the corresponding
    instantiated device's box.
    """
    if box.configuration.type == "projectClass":
        return get_topology().get_device(box.configuration.id).getBox(box.path)
    return box


class ParameterTreeWidget(QTreeWidget):
    # signalApplyChanged(box, enable, hasConflicts)
    signalApplyChanged = pyqtSignal(object, bool, bool)

    def __init__(self, conf=None):
        super(ParameterTreeWidget, self).__init__()

        self.conf = conf
        # Store previous selected item for tooltip handling
        self.prevItem = None

        self.setWordWrap(True)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.setSelectionMode(QAbstractItemView.ExtendedSelection)

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        # Actions from configurationPanel are added via addContextAction
        self.mContext = QMenu(self)
        self.customContextMenuRequested.connect(
            self.onCustomContextMenuRequested)

        self.model().setSupportedDragActions(Qt.CopyAction)
        self.setDragEnabled(True)

        self.header().setResizeMode(QHeaderView.ResizeToContents)

    def clear(self):
        """The items in the tree are children of this widget.
        They must be deleted by Qt, otherwise they'll still receive
        signals.
        """
        def _recursive_destroy(parent):
            for i in range(parent.childCount()):
                item = parent.child(i)
                item.destroy()
                _recursive_destroy(item)

        # Tell all the items to destroy themselves before clearing the widget
        _recursive_destroy(self.invisibleRootItem())
        super(ParameterTreeWidget, self).clear()

        for c in self.children():
            if isinstance(c, Widget):
                c.setParent(None)

    def ensureMiddleColumnWidth(self):
        """Set the minimum column width for the whole table to the current
        width of the middle column.
        """
        header = self.header()
        header.setMinimumSectionSize(header.sectionSize(1))

    def mousePressEvent(self, event):
        item = self.itemAt(event.pos())

        # Make sure the event was on a valid item
        if not item:
            return

        # Get the tree widget's x position
        treeX = self.header().sectionViewportPosition(0)

        # Get the x coordinate of the root item. It is required in order to
        # calculate the identation of the item
        rootX = self.visualItemRect(self.invisibleRootItem()).x()

        # Get the rectangle of the viewport occupied by the pressed item
        vRect = self.visualItemRect(item)

        # Calculate the x coordinate of the item
        itemX = treeX + vRect.x() - rootX

        # Get the rect surrounding the icon
        iconRect = QRect(itemX, vRect.y(), vRect.height(), vRect.height())

        if self.prevItem and (self.prevItem is not item):
            # Hide tooltip of former item
            self.prevItem.setToolTipDialogVisible(False)

        # Now check where the press event took place and handle it
        # correspondingly
        if iconRect.contains(event.pos()):
            self.prevItem = item
            self.prevItem.setToolTipDialogVisible(True)

        super(QTreeWidget, self).mousePressEvent(event)

    def keyPressEvent(self, event):
        """Make sure that the defined key events are handles properly
        """
        key_event = event.key()
        if key_event in (Qt.Key_Return, Qt.Key_Enter, Qt.Key_Escape):
            # Get the last focused widget and its position in global coords
            focus_widget = self.focusWidget()
            focus_pos = focus_widget.mapToGlobal(focus_widget.pos())
            # Convert the global position to an item
            item = self.itemAt(self.mapFromGlobal(focus_pos)
                               - QPoint(0, self.header().height()))

            # Act on that item
            if key_event == Qt.Key_Escape:
                self.decline_item_changes(item)
            else:
                self.apply_item_changes(item)
            return

        super(QTreeWidget, self).keyPressEvent(event)

    def mimeData(self, items):
        """Provide data for Drag & Drop operations.
        """
        if len(items) == 0 or self.conf.type == 'class':
            return None

        dragged = []
        for item in items:
            if isinstance(item.box.descriptor, ChoiceOfNodes):
                continue  # Skip ChoiceOfNodes

            # Get the box. "box" is in the project, "realbox" the
            # one on the device. They are the same if not from a project
            box = item.box
            realbox = getDeviceBox(box)
            if realbox.descriptor is not None:
                box = realbox

            # Collect the relevant information
            data = {
                'key': box.key(),
                'label': item.text(0),
            }

            factory = DisplayWidget.getClass(box)
            if factory is not None:
                data['display_widget_class'] = factory.__name__
            if item.editable_widget:
                factory = EditableWidget.getClass(box)
                if factory is not None:
                    data['edit_widget_class'] = factory.__name__
            # Add it to the list of dragged items
            dragged.append(data)

        if not dragged:
            return None

        mimeData = QMimeData()
        mimeData.setData('source_type', 'ParameterTreeWidget')
        mimeData.setData('tree_items', json.dumps(dragged))
        return mimeData

    def applyItem(self, item):
        """Return a list of pairs (box, value) to be applied
        """
        if not (item.is_editable and item.apply_enabled):
            return []
        # TODO: find out why value is same for all boxes
        return [(box, item.editable_widget.value)
                for box in item.editable_widget.boxes
                if box.isAccessible() and box.isAllowed()]

    def decline_all(self):
        """Decline either all changes or only the selected ones
        """
        nbSelectedItems = self.nbSelectedApplyEnabledItems()
        if nbSelectedItems > 0:
            [self.decline_item_changes(item) for item in self.selectedItems()]
        else:
            self.decline_all_changes()

    def nbSelectedApplyEnabledItems(self):
        """Return only selected items for not applied yet
        """
        counter = 0
        for item in self.selectedItems():
            if item.is_editable and item.apply_enabled:
                counter += 1
        return counter

    def setErrorState(self, inErrorState):
        self._r_setErrorStateItem(self.invisibleRootItem(), inErrorState)

    def setReadOnly(self, readOnly):
        self._r_setReadOnlyItem(self.invisibleRootItem(), readOnly)

    def _r_setErrorStateItem(self, item, inErrorState):
        for i in range(item.childCount()):
            childItem = item.child(i)
            childItem.setErrorState(inErrorState)
            self._r_setErrorStateItem(childItem, inErrorState)

    def _r_setReadOnlyItem(self, item, readOnly):
        for i in range(item.childCount()):
            childItem = item.child(i)
            childItem.setReadOnly(readOnly)
            self._r_setReadOnlyItem(childItem, readOnly)

    def addContextAction(self, action):
        self.mContext.addAction(action)

    def addContextMenu(self, menu):
        self.mContext.addMenu(menu)

    def addContextSeparator(self):
        self.mContext.addSeparator()

    def globalAccessLevelChanged(self):
        rootItem = self.invisibleRootItem()
        for i in range(rootItem.childCount()):
            self._r_globalAccessLevelChanged(rootItem.child(i))

    def _r_globalAccessLevelChanged(self, item):
        global_level = krb_globals.GLOBAL_ACCESS_LEVEL
        if item.requiredAccessLevel > global_level:
            item.setHidden(True)
        else:
            item.setHidden(False)

        if item.editable_widget is not None:
            # Let editable widgets make their own adjustments
            item.editable_widget.updateState()

        if isinstance(item, CommandTreeWidgetItem):
            item.updateState()

        if not item.isChoiceElement and not item.isListElement:
            for i in range(item.childCount()):
                self._r_globalAccessLevelChanged(item.child(i))

    def _r_applyAll(self, item, config):
        """Recursive function for tree to update the apply buttons
        """
        for i in range(item.childCount()):
            childItem = item.child(i)
            self.addItemDataToHash(childItem, config)
            self._r_applyAll(childItem, config)

    def onApplyChanged(self, box, enable):
        """ Called when apply button of an editable_widget changed
        Check if no apply button in tree is enabled/conflicted anymore
        """
        def recurse(item):
            # Returns a tuple containing the enabled and the conflicted
            # state
            for i in range(item.childCount()):
                childItem = item.child(i)
                if isinstance(childItem, PropertyTreeWidgetItem):
                    result = recurse(childItem)
                    if result[0]:  # Bug: returns but
                        return result

            if (not isinstance(item, PropertyTreeWidgetItem) or
                    not item.is_editable):
                return False, False

            return (item.apply_enabled, item.has_conflict)

        result = recurse(self.invisibleRootItem())
        self.signalApplyChanged.emit(box, result[0], result[1])

    def allItems(self):
        stack = []
        item = self.invisibleRootItem()
        while True:
            stack.extend(item.child(i) for i in range(item.childCount()))
            if not stack:
                return
            item = stack.pop()
            yield item

    def onApplyAll(self):
        nbSelectedItems = self.nbSelectedApplyEnabledItems()
        if nbSelectedItems > 0:
            selectedItems = self.selectedItems()
        else:
            selectedItems = self.allItems()
        boxes = sum([self.applyItem(item) for item in selectedItems], [])
        get_network().onReconfigure(boxes)

    def decline_all_changes(self):
        def recurse(item):
            for i in range(item.childCount()):
                childItem = item.child(i)
                self.decline_item_changes(childItem)
                recurse(childItem)

        recurse(self.invisibleRootItem())

    def onCustomContextMenuRequested(self, pos):
        item = self.itemAt(pos)
        if item is None:
            # Show standard context menu
            self.mContext.exec_(QCursor.pos())
            return

        item.showContextMenu()

    def apply_item_changes(self, item):
        if item is None or not item.is_editable:
            return
        item.apply_changes()

    def decline_item_changes(self, item):
        if item is None or not item.is_editable:
            return
        item.decline_changes()
