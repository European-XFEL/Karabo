#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on April 15, 2021
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from copy import deepcopy
from pathlib import Path

from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import QDialog, QDialogButtonBox

from karabo.common.enums import ProxyStatus
from karabo.native import Hash, create_html_hash
from karabogui.binding.api import (
    extract_init_configuration, validate_binding_configuration)

_NO_SCHEMA_STATUS = (ProxyStatus.MISSING, ProxyStatus.NOSERVER,
                     ProxyStatus.NOPLUGIN)


class DeviceConfigurationDialog(QDialog):
    """The basic device configuration dialog for a project device

    This QDialog shows the `Hash` configuration in a html format and can
    show the conflicts on request.
    """

    def __init__(self, name, configuration, project_device=None, parent=None):
        super().__init__(parent=parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        ui_file = str(Path(__file__).parent / "device_configuration.ui")
        uic.loadUi(ui_file, self)
        self.setWindowTitle(f"Configuration {name} "
                            f"- {project_device.device_id}")
        flags = Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint
        self.setWindowFlags(self.windowFlags() | flags)

        apply_button = self.ui_buttonBox.button(QDialogButtonBox.Apply)
        apply_button.clicked.connect(self.accept)
        discard_button = self.ui_buttonBox.button(QDialogButtonBox.Discard)
        discard_button.clicked.connect(self.reject)
        discard_button.setFocus()

        self._invalid_paths = []

        self.ui_erase_path.clicked.connect(self._erase_path)
        self.ui_erase_all.clicked.connect(self._erase_config)
        self.ui_sanitize.clicked.connect(self._sanitize_config)
        if project_device.status in _NO_SCHEMA_STATUS:
            self.ui_sanitize.setEnabled(False)
            text = "Sanitize is only available with a class schema"
            self.ui_sanitize.setToolTip(text)
        self.project_device = project_device
        # Make sure we deepcopy for manipulations!
        self.configuration = deepcopy(configuration)
        self._show_configuration()

    @Slot()
    def _sanitize_config(self):
        config = Hash()
        remove_attr = self.ui_remove_attributes.isChecked()
        for k, v, a in Hash.flat_iterall(self.configuration, empty=False):
            if k in self._invalid_paths:
                continue
            if remove_attr:
                config[k] = v
            else:
                config.setElement(k, v, a)

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
        self._update_validation_view()

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

    def _update_validation_view(self):
        widget = self.ui_valid_info
        proxy = self.project_device.get_class_proxy()
        binding = proxy.binding
        # No schema arrived yet!
        if not len(binding.value):
            html = "<center>No schema available!</center>"
            invalid = []
        else:
            config = validate_binding_configuration(
                binding=binding, config=self.configuration)
            if config.empty():
                html = "<center>No conflicts in configuration!</center>"
                invalid = []
            else:
                html = create_html_hash(config)
                invalid = config.paths(intermediate=False)

        self._invalid_paths = invalid
        widget.setHtml(html)
        scroll_bar = widget.verticalScrollBar()
        scroll_bar.setValue(scroll_bar.sliderPosition())
