#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on December 13, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from copy import copy

from PyQt4.QtCore import Qt
from PyQt4.QtGui import (QStandardItemModel, QVBoxLayout, QWidget,
                         QStandardItem, QLabel, QPushButton, QTreeView)

from karabo_gui.events import (KaraboBroadcastEvent, KaraboEventSender,
                               register_for_broadcasts,
                               unregister_from_broadcasts)
from karabo_gui.schema import Dummy
from karabo_gui.singletons.api import get_network
from .base import BasePanelWidget


class RunConfigPanel(BasePanelWidget):
    def __init__(self, instanceId, title):
        super(RunConfigPanel, self).__init__(title)

        self.instanceId = instanceId
        self.availableGroups = {}
        self.groupBox = None
        self.sendBox = None

        # Register for broadcasts
        # NOTE: unregister_from_broadcasts is called by closeEvent()
        register_for_broadcasts(self)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)

        self.groupList = QTreeView(parent=widget)
        self.headers = ['source', 'type', 'behavior', 'monitor', 'access']
        self.groupModel = QStandardItemModel(0, len(self.headers),
                                             self.groupList)

        self.groupModel.setHorizontalHeaderLabels(self.headers)

        self.groupModel.itemChanged.connect(self.onGroupItemChanged)
        self.groupList.setModel(self.groupModel)

        self.pbSend = QPushButton("Send to DAQ", parent=widget)
        self.pbSend.clicked.connect(self.onPushToDaq)

        main_layout = QVBoxLayout(widget)
        # main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.addWidget(QLabel("Run config. groups"))
        main_layout.addWidget(self.groupList)
        main_layout.addWidget(self.pbSend)

        return widget

    def closeEvent(self, event):
        unregister_from_broadcasts(self)

    def eventFilter(self, obj, event):
        """ Router for incoming broadcasts
        """
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.DeviceStateChanged:
                configuration = event.data.get('configuration')
                classId = configuration.classId
                cid = configuration.id
                if (classId == "RunConfigurator" and self.instanceId == cid):
                    self.updateConfig(configuration)
            elif event.sender is KaraboEventSender.RunConfigSourcesUpdate:
                senderId = event.data.get('instanceId')
                if senderId != self.instanceId:
                    return False
                group = event.data.get('group')
                sources = event.data.get('sources')
                self._updateDetails(group, sources)
            elif event.sender is KaraboEventSender.NetworkConnectStatus:
                if not event.data['status']:
                    self.groupList.destroy()
            return False
        return super(RunConfigPanel, self).eventFilter(obj, event)

    def updateConfig(self, config):
        """
        Called whenever a new configuration becomes available

        Will update the individual components of the panel if the configuration
        has updated information for these.
        """
        self.availableGroups = {}
        self.groupBox = config.getBox(["availableGroups"])

        groups = self.groupBox.value
        if not isinstance(groups, Dummy):
            for g in groups:
                self.availableGroups[g['groupId']] = g
            self._updateGroups()

        self.sendBox = config.getBox(["buildConfigurationInUse"])

    def _updateGroups(self):
        """
        Update the available run configuration groups list
        """
        for key, group in self.availableGroups.items():
            if len(self.groupModel.findItems(key)) > 0:
                continue
            else:
                item = QStandardItem(key)
                # add a checkbox to it
                item.setCheckable(True)
                if 'description' in group:
                    item.setToolTip(group['description'])
                self.groupModel.appendRow(item)
            get_network().onSourcesInGroup(self.instanceId, key)

    def _updateDetails(self, group, sources):
        """
        Update the configuration group details
        """
        items = self.groupModel.findItems(group)
        for item in items:
            existingSources = item.data(Qt.UserRole+1)
            if existingSources is None:
                existingSources = []
            for source in sources:
                attrs = ['source', 'type', 'behavior', 'monitored', 'access']
                row = [QStandardItem(str(source.get(a))) for a in attrs]
                if source.get('source') not in existingSources:
                    item.appendRow(row)
                    existingSources.append(source.get('source'))
            item.setData(existingSources, Qt.UserRole+1)

    def onGroupItemChanged(self, item):
        """
        Called whenever the checked selection in the available groups changes.

        This will lead to a reconfiguration request being sent to the
        run configurator device this widget reflects
        """
        updates = copy(self.availableGroups)

        for i in range(self.groupModel.rowCount()):
            item = self.groupModel.item(i)
            if item is not None and item.text() in self.availableGroups:
                updates[item.text()].set("use", bool(item.checkState()))

        updateList = [i for k, i in updates.items()]
        boxes = [(self.groupBox, updateList), ]
        get_network().onReconfigure(boxes)

    def onPushToDaq(self):
        """
        Push the compiled source list to the DAQ service
        """
        self.sendBox.execute()
