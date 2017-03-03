#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on March 3, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import (QLabel, QLineEdit, QPlainTextEdit, QPushButton,
                         QVBoxLayout, QWidget)

from karabo_gui.editablewidgets.editabletableelement import EditableTableElement  # noqa
from karabo_gui.events import (KaraboEventSender, register_for_broadcasts,
                               unregister_from_broadcasts)
import karabo_gui.icons as icons
from karabo_gui.schema import Box, Dummy
from karabo_gui.singletons.api import get_network, get_topology
from .base import BasePanelWidget


class RunConfigGroupModel:

    def __init__(self, config):
        self.update(config)

    def _verifyNotDummy(self, value):
        if isinstance(value, Dummy):
            return None
        return value

    @property
    def expertBox(self):
        return self._expertSourceBox

    @property
    def expert(self):
        return self._verifyNotDummy(self._expertSourceBox.value)

    @property
    def userBox(self):
        return self._userSourceBox

    @property
    def user(self):
        return self._verifyNotDummy(self._userSourceBox.value)

    @property
    def idBox(self):
        return self._idBox

    @property
    def groupId(self):
        return self._verifyNotDummy(self._idBox.value)

    @property
    def descBox(self):
        return self._descBox

    @property
    def description(self):
        return self._verifyNotDummy(self._descBox.value)

    def save(self, description=None, user=None, expert=None):
        """
        Saves the configuration back to the runconfig group device this model
        represents

        :param description: description of the group
        :param user: user configurable sources
        :param expert: expert configurable sources
        :return:
        """
        boxes = []
        if description:
            boxes.append((self._descBox, description))
        if user:
            boxes.append((self._userSourceBox, user))
        if expert:
            boxes.append((self._expertSourceBox, expert))

        get_network().onReconfigure(boxes)
        self._saveBox.execute()

    def update(self, config):
        """
        Update the model with config

        :param config: a configuration containing "id", "description", "user"
                       "expert" and a "saveGroupConfiguration" entry under a
                       "group" node
        """
        self._idBox = config.getBox(["group", "id"])
        self._descBox = config.getBox(["group", "description"])
        self._userSourceBox = config.getBox(["group", "user"])
        self._expertSourceBox = config.getBox(["group", "expert"])
        self._saveBox = config.getBox(["group", "saveGroupConfiguration"])


class RunConfigGroupPanel(BasePanelWidget):
    def __init__(self, instanceId, title):
        super(RunConfigGroupPanel, self).__init__(title)
        self.instanceId = instanceId
        self.model = None

        # make title slimmer by displaying icon instead
        # we split off only the last part of the instance id and add slashes
        self.fullTitle = title
        tparts = title.split("/")
        tlast = tparts[-1]
        self.shortTitle = "{}...".format(tlast[:min(6, len(tlast))])
        self.setWindowTitle(self.shortTitle)
        self.setWindowIcon(icons.runconfiggroup)
        self._setIconAndTooltip()

        # Register for broadcasts
        # NOTE: unregister_from_broadcasts is called by closeEvent()
        register_for_broadcasts(self)

        # make sure the device config has been requested
        device = get_topology().get_device(instanceId)
        get_network().onGetDeviceConfiguration(device.configuration)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)

        self.groupId = QLineEdit()
        self.groupId.setReadOnly(True)
        self.groupDescription = QPlainTextEdit()

        self.main_layout = QVBoxLayout(widget)
        self.main_layout.addWidget(QLabel("Group name"))
        self.main_layout.addWidget(self.groupId)
        self.main_layout.addWidget(QLabel("Description"))
        self.main_layout.addWidget(self.groupDescription)

        self.pbSend = QPushButton("Update Group", parent=widget)
        self.pbSend.clicked.connect(self._save)
        self.main_layout.addWidget(self.pbSend)

        bx = Box(("group", "user"), None, None)
        self.userSources = EditableTableElement(bx, self)
        self.userSourceLabel = QLabel("User Sources")
        self.main_layout.addWidget(self.userSourceLabel)
        self.main_layout.addWidget(self.userSources.widget)

        bx = Box(("group", "expert"), None, None)
        self.expertSources = EditableTableElement(bx, self)
        self.expertSourceLabel = QLabel("Expert Sources")
        self.main_layout.addWidget(self.expertSourceLabel)
        self.main_layout.addWidget(self.expertSources.widget)

        return widget

    def _setIconAndTooltip(self):
        if self.is_docked:
            self.panel_container.setTabIcon(self.index, icons.runconfiggroup)
            self.panel_container.setTabToolTip(self.index, self.fullTitle)

    def showEvent(self, event):
        super(RunConfigGroupPanel, self).showEvent(event)
        self.set_title(self.fullTitle)
        self._setIconAndTooltip()
        get_network().onStartMonitoringDevice(self.instanceId)

    def hideEvent(self, event):
        super(RunConfigGroupPanel, self).hideEvent(event)
        self.set_title(self.shortTitle)
        get_network().onStopMonitoringDevice(self.instanceId)

    def closeEvent(self, event):
        super(RunConfigGroupPanel, self).closeEvent(event)
        if event.isAccepted():
            unregister_from_broadcasts(self)

    def karaboBroadcastEvent(self, event):
        """ Router for incoming broadcasts
        """
        if event.sender is KaraboEventSender.DeviceStateChanged:
            configuration = event.data.get('configuration')
            check_class_id = configuration.classId == 'RunConfigurationGroup'
            check_instance_id = self.instanceId == configuration.id
            if (check_class_id and check_instance_id):
                self.updateConfig(configuration)
        return False

    def updateConfig(self, config):
        """
        Called whenever a new configuration becomes available

        Will update the individual components of the panel if the configuration
        has updated information for these.
        """

        if self.model is None:
            self.model = RunConfigGroupModel(config)

            # we can only fully render the tables when we've received the
            # first schema from the device
            self.main_layout.removeWidget(self.userSourceLabel)
            self.main_layout.removeWidget(self.userSources.widget)
            self.userSources.widget.close()
            self.userSources = EditableTableElement(self.model.userBox, self)
            self.main_layout.addWidget(self.userSourceLabel)
            self.main_layout.addWidget(self.userSources.widget)

            self.main_layout.removeWidget(self.expertSourceLabel)
            self.main_layout.removeWidget(self.expertSources.widget)
            self.expertSources.widget.close()
            self.expertSources = EditableTableElement(self.model.expertBox,
                                                      self)

            self.main_layout.addWidget(self.expertSourceLabel)
            self.main_layout.addWidget(self.expertSources.widget)

        else:
            self.model.update(config)

        if self.model.groupId:
            self.groupId.setText(self.model.groupId)

        if self.model.description:
            self.groupDescription.setPlainText(self.model.description)

        if self.model.user:
            self.userSources.valueChanged(self.model.userBox,
                                          self.model.user)

        if self.model.expert:
            self.expertSources.valueChanged(self.model.expertBox,
                                            self.model.expert)

    @pyqtSlot()
    def _save(self):
        """
        Push the compiled source list to the config group device
        """
        description = self.groupDescription.toPlainText()
        user = self.userSources.tableModel.getHashList()
        expert = self.expertSources.tableModel.getHashList()
        self.model.save(description=description, user=user, expert=expert)
