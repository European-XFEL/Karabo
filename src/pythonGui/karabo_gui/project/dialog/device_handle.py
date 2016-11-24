#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtGui import QDialog

from karabo.middlelayer import AccessLevel, Hash
import karabo_gui.globals as krb_globals
from karabo_gui.topology import Manager


class DeviceHandleDialog(QDialog):
    def __init__(self, model=None, parent=None):
        super(DeviceHandleDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'device_handle.ui')
        uic.loadUi(filepath, self)

        # XXX Get available plugins from systemTopology
        for class_id in self._get_available_plugins():
            self.cbClass.addItem(class_id)

        if model is None:
            title = 'Add device'
        else:
            title = 'Edit device'
            self.leTitle.setText(model.instance_id)
            # XXX class_id is part of the DeviceConfigurationModel
            #model.configs
            #index = self.cbClass.findText(model.config[0].class_id)
            #self.cbClass.setCurrentIndex(index)
            index = self.cbIfExists.findText(model.if_exists)
            self.cbIfExists.setCurrentIndex(index)
            self.teDescription.setPlainText(model.description)
        self.setWindowTitle(title)

    def _get_available_plugins(self):
        """ Get all available plugins of `systemTopology`"""
        available_plugins = []
        servers = Manager().systemHash.get('server', Hash())
        for server_id, _, attrs in servers.iterall():
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
    def description(self):
        return self.teDescription.toPlainText()
