#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QDialog

from karabo.middlelayer import AccessLevel, Hash
import karabo_gui.globals as krb_globals
from karabo_gui.singletons.api import get_manager


class DeviceHandleDialog(QDialog):
    def __init__(self, server_id=None, model=None, parent=None):
        super(DeviceHandleDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'device_handle.ui')
        uic.loadUi(filepath, self)

        # Get available plugins from systemTopology
        for class_id in self._get_available_plugins():
            self.cbClass.addItem(class_id)

        self.leServerId.setText(server_id)

        if model is None:
            title = 'Add device'
            self.gbConfig.setEnabled(False)
        else:
            title = 'Edit device'
            self.leTitle.setText(model.instance_id)
            # Map UUIDs to list of available DeviceConfigurationModels
            uuid_configs = {}
            for config in model.configs:
                uuid_configs.setdefault(config.uuid, []).append(config)

            for uuid, configs in uuid_configs.items():
                # Add UUID to combobox and add list of device configs as itemData
                self.cbConfig.addItem(uuid, configs)

            # XXX TODO the active revision number is always 0
            active_uuid, active_rev = model.active_config_ref
            dev_conf = model.select_config(active_uuid, active_rev)
            if dev_conf is not None:
                self._update_config_widgets(dev_conf)

            # Make sure the signal is triggered when setting the index below
            self.cbConfig.setCurrentIndex(-1)
            # Update revisions combobox if configuration changes
            self.cbConfig.currentIndexChanged[int].connect(self.config_changed)
            index = self.cbConfig.findText(active_uuid)
            self.cbConfig.setCurrentIndex(index)

            index = self.cbIfExists.findText(model.if_exists)
            self.cbIfExists.setCurrentIndex(index)
        self.setWindowTitle(title)

    def _get_available_plugins(self):
        """ Get all available plugins of `systemTopology`"""
        available_plugins = []
        servers = get_manager().systemHash.get('server', Hash())
        for _, _, attrs in servers.iterall():
            if not attrs:
                continue

            for class_id, visibility in zip(attrs.get("deviceClasses", []),
                                            attrs.get("visibilities", [])):
                # Only show accessible plugins depending on global access level
                if AccessLevel(visibility) >= krb_globals.GLOBAL_ACCESS_LEVEL:
                    continue
                if class_id not in available_plugins:
                    available_plugins.append(class_id)
        return available_plugins

    def _update_config_widgets(self, dev_config_model):
        """ Update all relevant widgets which show `DeviceConfigurationModel`
        information

        :param dev_config_model: The `DeviceConfgurationModel` which data
                                 should be displayed.
        """
        index = self.cbClass.findText(dev_config_model.class_id)
        self.cbClass.setCurrentIndex(index)
        self.teDescription.setPlainText(dev_config_model.description)
        index = self.cbVersion.findText(str(dev_config_model.revision))
        self.cbVersion.setCurrentIndex(index)

    @pyqtSlot(int)
    def config_changed(self, index):
        configs = self.cbConfig.itemData(index)
        self.cbVersion.clear()
        self.cbVersion.addItems([str(conf.revision) for conf in configs])

        if configs:
            # Update dialog to active configuration data
            active_config = configs[-1]
            self._update_config_widgets(active_config)

    @property
    def instance_id(self):
        return self.leTitle.text()

    @property
    def class_id(self):
        return self.cbClass.currentText()

    @property
    def if_exists(self):
        return self.cbIfExists.currentText()

    @property
    def new_config(self):
        return not self.gbConfig.isChecked()

    @property
    def active_uuid(self):
        return self.cbConfig.currentText()

    @property
    def active_revision(self):
        return int(self.cbVersion.currentText())

    @property
    def description(self):
        return self.teDescription.toPlainText()
