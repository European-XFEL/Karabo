#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2016
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from qtpy import uic
from qtpy.QtCore import Slot
from qtpy.QtWidgets import QDialog, QDialogButtonBox

from karabogui.singletons.api import get_topology
from karabogui.util import InputValidator

from .utils import get_dialog_ui


class DeviceHandleDialog(QDialog):
    def __init__(self, server_id=None, model=None, add_config=False,
                 is_online=False, class_id='', parent=None):
        """ A dialog to configure device configurations

        :param server_id: The ID of the server the device configuration belongs
                          to
        :param model: The ``DeviceInstanceModel`` object
        :param add_config: A boolean which describes whether a new
                           ``DeviceConfigurationModel`` should be added
        :param is_online: A boolean which is True if the device being edited
                          is currently online.
        :param class_id: A string containing the class_id of the class which
                         must be used by the device.
        :param parent: The parent of the dialog
        """
        super().__init__(parent)
        uic.loadUi(get_dialog_ui('device_handle.ui'), self)
        validator = InputValidator(parent=self)
        self.leTitle.setValidator(validator)

        # Get available plugins from systemTopology
        for cls_id in self._get_available_plugins(server_id):
            self.cbClass.addItem(cls_id)
        self.leServerId.setText(server_id)

        # Disable the instance_id editor when the device is online
        self.leTitle.setEnabled(not is_online)

        if model is None:
            title = 'Add device'
            self.cbConfig.setEditable(True)
            self.cbConfig.lineEdit().setText('default')
            # We do not allow to modify 'default' configuration!
            self.cbConfig.setEnabled(False)

            # If we already know the class, select it. It is offline
            if class_id != '':
                self._update_plugin_widget(class_id)
                self.cbClass.setEnabled(False)
        else:
            active_dev_conf = model.select_config(model.active_config_ref)
            if add_config:
                title = 'Add device configuration'
                self.cbConfig.setEditable(True)
                line_edit = self.cbConfig.lineEdit()
                line_edit.setValidator(InputValidator(parent=self))
                line_edit.setFocus()

                # These widgets belong to a ``DeviceInstanceModel`` and
                # should not be changed in case a configuration is added
                self.leTitle.setEnabled(False)
                if active_dev_conf is not None:
                    self._update_plugin_widget(active_dev_conf.class_id)
                self.cbClass.setEnabled(False)

            else:
                title = 'Edit device configuration'
                self._init_config_widgets(model)
                self.cbConfig.setEnabled(not is_online)
                if active_dev_conf is not None:
                    self._update_config_widgets(active_dev_conf)
                    index = self.cbConfig.findText(active_dev_conf.simple_name)
                    self.cbConfig.setCurrentIndex(index)

                self.cbClass.setEnabled(not is_online)

            self.leTitle.setText(model.instance_id)

        self.setWindowTitle(title)
        self.leTitle.textChanged.connect(self._update_button_box)
        self.cbConfig.currentIndexChanged.connect(self._update_button_box)
        self.cbConfig.editTextChanged.connect(self._update_button_box)
        self.cbClass.currentIndexChanged.connect(self._update_button_box)
        self._update_button_box()

    def _init_config_widgets(self, dev_inst_model):
        """ Init all device configuration related widgets to the associated
        ``dev_inst_model``

        :param dev_inst_model: The ``DeviceInstanceModel`` object
        """
        for config in dev_inst_model.configs:
            # Add simple_name to combobox and add uuid of config
            self.cbConfig.addItem(config.simple_name, config)

        # Make sure the signal is triggered when setting the index below
        self.cbConfig.setCurrentIndex(-1)
        # Update if configuration changes
        self.cbConfig.currentIndexChanged[int].connect(self.config_changed)

    def _get_available_plugins(self, device_server_id):
        """Get all available plugins of `systemTopology` for the given
        ``device_server_id``
        """
        attrs = get_topology().get_attributes(f"server.{device_server_id}")
        attrs = attrs if attrs is not None else {}
        available_plugins = attrs.get("deviceClasses", [])
        return sorted(available_plugins)

    def _update_config_widgets(self, dev_config_model):
        """ Update all relevant widgets which show `DeviceConfigurationModel`
        information

        :param dev_config_model: The `DeviceConfgurationModel` which data
                                 should be displayed.
        """
        self._update_plugin_widget(dev_config_model.class_id)

    def _update_plugin_widget(self, class_id):
        index = self.cbClass.findText(class_id)
        self.cbClass.setCurrentIndex(index)

    @Slot()
    def _update_button_box(self):
        """Only enable Ok button, if title and configuration is set
        """
        enabled = (len(self.leTitle.text()) and
                   len(self.cbConfig.currentText()) and
                   len(self.cbClass.currentText()))
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

    @Slot(int)
    def config_changed(self, index):
        config_model = self.cbConfig.itemData(index)
        # Update dialog to active configuration data
        self._update_config_widgets(config_model)

    @property
    def instance_id(self):
        return self.leTitle.text()

    @property
    def class_id(self):
        return self.cbClass.currentText()

    @property
    def active_uuid(self):
        index = self.cbConfig.currentIndex()
        config_model = self.cbConfig.itemData(index)
        return config_model.uuid

    @property
    def configuration_name(self):
        return self.cbConfig.currentText()

    @property
    def description(self):
        return ""
