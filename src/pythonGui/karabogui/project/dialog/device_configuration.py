#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on April 15, 2021
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
from copy import deepcopy

from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import QDialog, QDialogButtonBox

from karabo.native import Hash, create_html_hash
from karabogui.binding.api import (
    ProxyStatus, extract_init_configuration, validate_binding_configuration)

from .utils import get_dialog_ui

_NO_SCHEMA_STATUS = (ProxyStatus.MISSING, ProxyStatus.NOSERVER,
                     ProxyStatus.NOPLUGIN)


class DeviceConfigurationDialog(QDialog):
    """The basic device configuration dialog for a project device

    This QDialog shows the `Hash` configuration in a html format and can
    show the conflicts on request.
    """

    def __init__(self, name, configuration, project_device=None,
                 editable=False, parent=None):
        super().__init__(parent=parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        uic.loadUi(get_dialog_ui("device_configuration.ui"), self)
        self.setWindowTitle(f"Configuration {name} "
                            f"- {project_device.device_id}")
        flags = Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint
        self.setWindowFlags(self.windowFlags() | flags)

        apply_button = self.ui_buttonBox.button(QDialogButtonBox.Apply)
        apply_button.clicked.connect(self.accept)
        discard_button = self.ui_buttonBox.button(QDialogButtonBox.Close)
        discard_button.clicked.connect(self.reject)
        discard_button.setFocus()

        if not editable:
            self.ui_edit_group.setVisible(False)
            self.ui_buttonBox.button(QDialogButtonBox.Apply).setVisible(False)

        self._invalid_paths = []
        self.ui_erase_path.clicked.connect(self._erase_path)
        self.ui_erase_all.clicked.connect(self._erase_config)
        self.ui_cleanup.clicked.connect(self._cleanup_configuration)
        if project_device.status in _NO_SCHEMA_STATUS:
            self.ui_cleanup.setEnabled(False)
            text = "Cleanup is only available with a class schema"
            self.ui_cleanup.setToolTip(text)
        self.project_device = project_device
        # Make sure we deepcopy for manipulations!
        self.configuration = deepcopy(configuration)
        self._show_configuration()

    @Slot()
    def _cleanup_configuration(self):
        config = Hash()
        for k, v, a in Hash.flat_iterall(self.configuration, empty=False):
            if k in self._invalid_paths:
                continue
            config[k] = v

        proxy = self.project_device.get_class_proxy()
        binding = proxy.binding
        # Schema is available, filter out read only properties without
        # run time attributes and remove attrs that are not runtime attributes
        if len(binding.value):
            config = extract_init_configuration(
                binding=binding, config=config)

        self.configuration = config
        self._show_configuration()

    @Slot()
    def _erase_config(self):
        """Clear the full configuration"""
        self.configuration = Hash()
        self._show_configuration()

    @Slot()
    def _erase_path(self):
        path = self.ui_paths.currentText()
        index = self.ui_paths.currentIndex()
        if path in self.configuration:
            self.configuration.erase(path)
        self._show_configuration()
        if self.ui_paths.count() > index:
            self.ui_paths.setCurrentIndex(index)

    @Slot()
    def _show_configuration(self):
        self._update_configuration_view()
        self._extract_invalid_paths()

    def _update_configuration_view(self):
        widget = self.ui_text_info
        config = self.configuration
        # Note: Protect as well against a `None` configuration!
        if config is None or config.empty():
            html = "<center>No changes in configuration!</center>"
            self.ui_paths.clear()
            self.ui_paths.setEnabled(False)
            self.ui_erase_path.setEnabled(False)
            self.ui_num_paths.setText("# 0")
        else:
            html = create_html_hash(config)
            # Enable to erase all path, broken and valid ones!
            self.ui_paths.clear()
            self.ui_paths.setEnabled(True)
            self.ui_erase_path.setEnabled(True)
            paths = config.paths(intermediate=True)
            self.ui_num_paths.setText(f"# {len(paths)}")
            self.ui_paths.addItems(paths)

        widget.setHtml(html)
        scroll_bar = widget.verticalScrollBar()
        scroll_bar.setValue(scroll_bar.sliderPosition())

    def _extract_invalid_paths(self):
        proxy = self.project_device.get_class_proxy()
        binding = proxy.binding
        # No schema arrived yet!
        if not len(binding.value):
            invalid = []
        else:
            config = validate_binding_configuration(
                binding=binding, config=self.configuration)
            if config.empty():
                invalid = []
            else:
                invalid = config.paths(intermediate=False)

        self._invalid_paths = invalid
