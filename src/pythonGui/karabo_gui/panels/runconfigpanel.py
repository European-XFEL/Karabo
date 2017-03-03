#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on December 13, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from copy import copy

from PyQt4.QtCore import Qt
from PyQt4.QtGui import (QStandardItemModel, QVBoxLayout, QWidget,
                         QStandardItem, QLabel, QPushButton, QTreeView)


from karabo_gui.events import (
    KaraboEventSender, register_for_broadcasts, unregister_from_broadcasts)
import karabo_gui.icons as icons
from karabo_gui.schema import Dummy
from karabo_gui.singletons.api import get_network, get_topology
from .base import BasePanelWidget


class RunConfigPanel(BasePanelWidget):
    def __init__(self, instanceId, title):
        super(RunConfigPanel, self).__init__(title)

        self.instanceId = instanceId
        self.availableGroups = {}
        self.groupBox = None
        self.sendBox = None
        self.title = title
        self.setWindowIcon(icons.runconfig)
        self._setIconAndTooltip()

        # Register for broadcasts
        # NOTE: unregister_from_broadcasts is called by closeEvent()
        register_for_broadcasts(self)

        get_topology().get_device(instanceId)

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

    def _setIconAndTooltip(self):
        if self.is_docked:
            self.panel_container.setTabIcon(self.index, icons.runconfig)
            self.panel_container.setTabToolTip(self.index, self.title)

    def showEvent(self, event):
        super(RunConfigPanel, self).showEvent(event)
        self._setIconAndTooltip()
        get_network().onStartMonitoringDevice(self.instanceId)

    def hideEvent(self, event):
        super(RunConfigPanel, self).hideEvent(event)
        get_network().onStopMonitoringDevice(self.instanceId)

    def closeEvent(self, event):
        super(RunConfigPanel, self).closeEvent(event)
        if event.isAccepted():
            unregister_from_broadcasts(self)

    def karaboBroadcastEvent(self, event):
        """ Router for incoming broadcasts
        """
        sender = event.sender
        data = event.data
        if sender is KaraboEventSender.DeviceStateChanged:
            configuration = data.get('configuration')
            check_class_id = configuration.classId == 'RunConfigurator'
            check_instance_id = self.instanceId == configuration.id
            if (check_class_id and check_instance_id):
                self.updateConfig(configuration)
        elif sender is KaraboEventSender.RunConfigSourcesUpdate:
            senderId = data.get('instanceId')
            if senderId != self.instanceId:
                return False
            group = data.get('group')
            sources = data.get('sources')
            self._updateDetails(group, sources)
        elif sender is KaraboEventSender.NetworkConnectStatus:
            if not data['status']:
                self.groupList.destroy()
        return False

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
                src = source.get('source')
                if src not in existingSources:
                    row = [QStandardItem(str(source.get(a))) for a in attrs]
                    item.appendRow(row)
                    existingSources.append(src)
                else:
                    for row in range(item.rowCount()):
                        child = item.child(row)
                        if child.text() != src:
                            continue
                        for column, a in enumerate(attrs):
                            new_item = QStandardItem(str(source.get(a)))
                            item.setChild(row, column, new_item)
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
