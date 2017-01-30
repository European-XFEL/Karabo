#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QDialog, QDialogButtonBox

from karabo.middlelayer import AccessLevel
import karabo_gui.globals as krb_globals
from karabo_gui.singletons.api import get_topology


class DeviceHandleDialog(QDialog):
    def __init__(self, server_id=None, model=None, add_config=False,
                 is_online=False, parent=None):
        """ A dialog to configure device configurations

        :param server_id: The ID of the server the device configuration belongs
                          to
        :param model: The ``DeviceInstanceModel`` object
        :param add_config: A boolean which describes whether a new
                           ``DeviceConfigurationModel`` should be added
        :param is_online: A boolean which is True if the device being edited
                          is currently online.
        :param parent: The parent of the dialog
        """
        super(DeviceHandleDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'device_handle.ui')
        uic.loadUi(filepath, self)

        self._initUI(server_id, model, add_config, is_online)

    def _initUI(self, server_id, model, add_config, is_online):
        # Get available plugins from systemTopology
        for class_id in self._get_available_plugins(server_id):
            self.cbClass.addItem(class_id)
        self.leServerId.setText(server_id)

        # Disable the instance_id editor when the device is online
        self.leTitle.setEnabled(not is_online)

        if model is None:
            title = 'Add device configuration'
            self._update_widgets_to_add_config()
            self.cbConfig.lineEdit().setText('default')
        else:
            active_uuid, active_rev = model.active_config_ref
            active_dev_conf = model.select_config(active_uuid, active_rev)
            if add_config:
                title = 'Add device configuration'
                self._update_widgets_to_add_config()

                # These widgets belong to a ``DeviceInstanceModel`` and
                # should not be changed in case a configuration is added
                self.leTitle.setEnabled(False)
                self.cbIfExists.setEnabled(False)
                if active_dev_conf is not None:
                    self._update_plugin_widget(active_dev_conf.class_id)
            else:
                title = 'Edit device configuration'
                self._init_config_widgets(model)
                if active_dev_conf is not None:
                    self._update_config_widgets(active_dev_conf)
                    index = self.cbConfig.findText(active_dev_conf.alias)
                    self.cbConfig.setCurrentIndex(index)

            self.cbClass.setEnabled(False)
            self.leTitle.setText(model.instance_id)
            index = self.cbIfExists.findText(model.if_exists)
            self.cbIfExists.setCurrentIndex(index)

        self.setWindowTitle(title)
        self.leTitle.textChanged.connect(self._update_button_box)
        self.cbConfig.editTextChanged.connect(self._update_button_box)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

    def _init_config_widgets(self, dev_inst_model):
        """ Init all device configuration related widgets to the associated
        ``dev_inst_model``

        :param dev_inst_model: The ``DeviceInstanceModel`` object
        """
        # Map UUIDs to list of available DeviceConfigurationModels
        uuid_configs = {}
        for config in dev_inst_model.configs:
            key = (config.uuid, config.alias)
            uuid_configs.setdefault(key, []).append(config)

        for key, configs in uuid_configs.items():
            uuid, alias = key
            # Add alias to combobox and add tuple of uuid and list of
            # device configs
            self.cbConfig.addItem(alias, (uuid, configs))

        # Make sure the signal is triggered when setting the index below
        self.cbConfig.setCurrentIndex(-1)
        # Update revisions combobox if configuration changes
        self.cbConfig.currentIndexChanged[int].connect(self.config_changed)

    def _get_available_plugins(self, device_server_id):
        """ Get all available plugins of `systemTopology` for the given
        ``device_server_id``"""
        available_plugins = set()

        def filter(server_id, attrs):
            if device_server_id != server_id:
                return False
            for class_id, visibility in zip(attrs.get('deviceClasses', []),
                                            attrs.get('visibilities', [])):
                # Only show accessible plugins depending on global access level
                if AccessLevel(visibility) >= krb_globals.GLOBAL_ACCESS_LEVEL:
                    continue
                available_plugins.add(class_id)
            return False

        get_topology().search_system_tree('server', filter)
        return list(available_plugins)

    def _update_widgets_to_add_config(self):
        """ Whenever a ``DeviceConfigurationModel`` is added, configuration
        related widgets need to be made editable or hidden.
        """
        self.cbConfig.setEditable(True)
        self.laVersion.setVisible(False)
        self.cbVersion.setVisible(False)

    def _update_config_widgets(self, dev_config_model):
        """ Update all relevant widgets which show `DeviceConfigurationModel`
        information

        :param dev_config_model: The `DeviceConfgurationModel` which data
                                 should be displayed.
        """
        self._update_plugin_widget(dev_config_model.class_id)
        self.teDescription.setPlainText(dev_config_model.description)
        index = self.cbVersion.findData(dev_config_model.revision)
        self.cbVersion.setCurrentIndex(index)

    def _update_plugin_widget(self, class_id):
        index = self.cbClass.findText(class_id)
        self.cbClass.setCurrentIndex(index)

    def _update_button_box(self, text):
        """ Only enable Ok button, if title and configuration alias is set"""
        enabled = (len(self.leTitle.text()) > 0 and
                   len(self.cbConfig.currentText()) > 0)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

    @pyqtSlot(int)
    def config_changed(self, index):
        uuid, configs = self.cbConfig.itemData(index)
        self.cbVersion.clear()
        for conf in configs:
            alias = conf.alias
            revision = conf.revision
            alias = conf.alias
            self.cbVersion.addItem('{} <{}>'.format(alias, revision), revision)

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
    def active_uuid(self):
        index = self.cbConfig.currentIndex()
        uuid, configs = self.cbConfig.itemData(index)
        return uuid

    @property
    def active_revision(self):
        index = self.cbVersion.currentIndex()
        revision = self.cbVersion.itemData(index)
        return revision

    @property
    def configuration_name(self):
        return self.cbConfig.currentText()

    @property
    def description(self):
        return self.teDescription.toPlainText()
