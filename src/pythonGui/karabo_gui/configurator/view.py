#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 3, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import QAbstractItemView, QTreeView

from karabo_gui.events import (
    KaraboEventSender, register_for_broadcasts, unregister_from_broadcasts)
from .qt_item_model import ConfigurationTreeModel
from .slot_delegate import SlotButtonDelegate
from .value_delegate import ValueDelegate


class ConfigurationTreeView(QTreeView):
    """A tree view for `Configuration` instances
    """
    signalApplyChanged = pyqtSignal()
    itemSelectionChanged = pyqtSignal()

    def __init__(self, conf=None, parent=None):
        super(ConfigurationTreeView, self).__init__(parent)
        self.setDragEnabled(True)
        self.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)

        # For compatibility with ConfigurationPanel
        self.conf = conf

        model = ConfigurationTreeModel(parent=self)
        self.setModel(model)
        model.configuration = conf

        # Add a delegate for rows with slot buttons
        delegate = SlotButtonDelegate(parent=self)
        self.setItemDelegateForColumn(0, delegate)
        # ... and a delegate for the Value column
        delegate = ValueDelegate(parent=self)
        self.setItemDelegateForColumn(1, delegate)

        # Don't forget to unregister!
        register_for_broadcasts(self)

    # ------------------------------------
    # Event handlers

    def closeEvent(self, event):
        event.accept()
        unregister_from_broadcasts(self)

    def karaboBroadcastEvent(self, event):
        sender = event.sender
        if sender is KaraboEventSender.AccessLevelChanged:
            model = self.model()
            config = model.configuration
            model.configuration = None
            model.configuration = config

        return False

    # ------------------------------------
    # NOTE: All of these methods are to stay compatible with the existing
    # configurator panel code.

    def addContextAction(self, action):
        pass

    def addContextMenu(self, menu):
        pass

    def addContextSeparator(self):
        pass

    def clear(self):
        self.model().configuration = None

    def decline_all(self):
        pass

    def decline_all_changes(self):
        pass

    def decline_item_changes(self, item):
        pass

    def globalAccessLevelChanged(self):
        pass

    def hideColumn(self, col):
        pass

    def onApplyAll(self):
        pass

    def nbSelectedApplyEnabledItems(self):
        """Return only selected items for not applied yet
        """
        return 0

    def selectedItems(self):
        return []

    def setErrorState(self, inErrorState):
        pass

    def setHeaderLabels(self, labels):
        pass

    def setReadOnly(self, readOnly):
        pass
