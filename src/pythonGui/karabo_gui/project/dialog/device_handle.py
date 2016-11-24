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
from karabo_gui.topology import Manager


class DeviceHandleDialog(QDialog):
    def __init__(self, server_id=None, model=None, parent=None):
        super(DeviceHandleDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'device_handle.ui')
        uic.loadUi(filepath, self)

        # XXX Get available plugins from systemTopology
        for class_id in self._get_available_plugins():
            self.cbClass.addItem(class_id)

        self.leServerId.setText(server_id)

        if model is None:
            title = 'Add device'
            self.gbConfig.setEnabled(False)
            self.cbConfig.setEnabled(False)
            self.cbVersion.setEnabled(False)
        else:
            title = 'Edit device'
            self.leTitle.setText(model.instance_id)

            # XXX TODO There is a toggle missing to decide whether to choose an
            # existing configuration or add a new one (self.gbConfig...)
            for config in model.configs:
                # XXX TODO there can be the same uuids in the list of configs
                # and revisions - here we need to get a mapping of uuid to
                # available revisions
                if (config.uuid, config.revision) == model.active_config_ref:
                    index = self.cbClass.findText(config.class_id)
                    self.cbClass.setCurrentIndex(index)
                    self.teDescription.setPlainText(config.description)
                self.cbConfig.addItem(config.uuid, config.revision)

            # Make sure the signal is triggered when setting the index below
            self.cbConfig.setCurrentIndex(-1)
            # Update revisions combobox if configuration changes
            self.cbConfig.currentIndexChanged[int].connect(self.config_changed)
            uuid, revision = model.active_config_ref
            index = self.cbConfig.findText(uuid)
            self.cbConfig.setCurrentIndex(index)
            index = self.cbVersion.findText(str(revision))
            self.cbVersion.setCurrentIndex(index)

            index = self.cbIfExists.findText(model.if_exists)
            self.cbIfExists.setCurrentIndex(index)
        self.setWindowTitle(title)

    def _get_available_plugins(self):
        """ Get all available plugins of `systemTopology`"""
        available_plugins = []
        servers = Manager().systemHash.get('server', Hash())
        for _, _, attrs in servers.iterall():
            if not attrs:
                continue

            for class_id, visibility in zip(attrs.get("deviceClasses", []),
                                            attrs.get("visibilities", [])):
                # Only show accessible plugins depending on global access level
                if AccessLevel(visibility) > krb_globals.GLOBAL_ACCESS_LEVEL:
                    continue
                if class_id not in available_plugins:
                    available_plugins.append(class_id)
        return available_plugins

    @pyqtSlot(int)
    def config_changed(self, index):
        revision = self.cbConfig.itemData(index)
        self.cbVersion.clear()
        self.cbVersion.addItem(str(revision))

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
    def active_uuid(self):
        return self.cbConfig.currentText()

    @property
    def active_revision(self):
        return int(self.cbVersion.currentText())

    @property
    def description(self):
        return self.teDescription.toPlainText()
