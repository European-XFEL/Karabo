#############################################################################
# Author: <dennis.goeeries@xfel.eu>
# Created on February 12, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QAbstractItemView, QTreeView

from karabogui.events import broadcast_event, KaraboEventSender
from karabogui.singletons.api import get_selection_tracker
from karabogui.util import set_treeview_header

from .device_model import DeviceTreeModel
from .tools import DeviceSceneHandler


class DeviceTreeView(QTreeView):
    def __init__(self, parent):
        super(DeviceTreeView, self).__init__(parent)
        self._selected_proxy = None  # A BaseDeviceProxy

        model = DeviceTreeModel(parent=self)
        self.setModel(model)
        self.setSelectionModel(model.selectionModel)
        model.rowsInserted.connect(self._items_added)
        model.signalItemChanged.connect(self.onSelectionChanged)

        set_treeview_header(self)

        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)

        self.handler_list = [DeviceSceneHandler()]
        self.expanded = True
        self.header().sectionDoubleClicked.connect(self.onDoubleClickHeader)

    def currentIndex(self):
        return self.model().currentIndex()

    def clear(self):
        self._selected_proxy = None
        self.clearSelection()
        self.model().clear()

    def scrollTo(self, index, hint=QAbstractItemView.EnsureVisible):
        """Reimplementation of the Qt function
        """
        self.setExpanded(index, True)
        super(DeviceTreeView, self).scrollTo(index)

    # ----------------------------
    # Events

    def mouseDoubleClickEvent(self, event):
        index = self.currentIndex()
        node = self.model().index_ref(index)
        # QModelIndexes tend to outlive the node objects which they reference!
        if node is None:
            return
        info = node.info()
        for handler in self.handler_list:
            if handler.can_handle(info):
                handler.handle(info)
                event.accept()
                return

        super(DeviceTreeView, self).mouseDoubleClickEvent(event)

    # ----------------------------
    # Slots

    def _items_added(self, parent_index, start, end):
        """React to the addition of an item (or items).
        """
        # Bail immediately if not the first item
        if start != 0:
            return

        self.expand(parent_index)

    @pyqtSlot(str, object)
    def onSelectionChanged(self, item_type, proxy):
        """Called by the data model when an item is selected
        """
        self._selected_proxy = proxy

        # Grab control of the global selection
        get_selection_tracker().grab_selection(self.model().selectionModel)

        # Tell the configurator
        if item_type not in ('class', 'device'):
            # servers and hosts clear the configurator
            proxy = None
        broadcast_event(KaraboEventSender.ShowConfiguration, {'proxy': proxy})

    @pyqtSlot()
    def onDoubleClickHeader(self):
        if self.expanded:
            self.collapseAll()
        else:
            self.expandAll()
        self.expanded = not self.expanded
