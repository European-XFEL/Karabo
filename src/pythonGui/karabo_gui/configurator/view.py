#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 3, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import QTreeView

from .qt_item_model import ConfigurationTreeModel


class ConfigurationTreeView(QTreeView):
    """A tree view for `Configuration` instances
    """
    signalApplyChanged = pyqtSignal()
    itemSelectionChanged = pyqtSignal()

    def __init__(self, conf=None, parent=None):
        super(ConfigurationTreeView, self).__init__(parent)

        # For compatibility with ConfigurationPanel
        self.conf = conf

        model = ConfigurationTreeModel(parent=self)
        self.setModel(model)
        model.configuration = conf

    # ------------------------------------
    # NOTE: All of these methods are to stay compatible with the existing
    # configurator panel code.

    def clear(self):
        self.model().configuration = None

    def setHeaderLabels(self, labels):
        pass

    def hideColumn(self, col):
        pass

    def decline_all(self):
        pass

    def nbSelectedApplyEnabledItems(self):
        """Return only selected items for not applied yet
        """
        return 0

    def selectedItems(self):
        return []

    def setErrorState(self, inErrorState):
        pass

    def setReadOnly(self, readOnly):
        pass

    def addContextAction(self, action):
        pass

    def addContextMenu(self, menu):
        pass

    def addContextSeparator(self):
        pass

    def globalAccessLevelChanged(self):
        pass

    def onApplyAll(self):
        pass

    def decline_all_changes(self):
        pass

    def decline_item_changes(self, item):
        pass
