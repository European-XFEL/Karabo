#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on April 15, 2021
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from pathlib import Path

from qtpy import uic
from qtpy.QtCore import Slot, Qt
from qtpy.QtWidgets import QDialog

from karabogui.binding.api import validate_binding_configuration
from karabo.native import create_html_hash


class DeviceConfigurationDialog(QDialog):
    """The basic device configuration dialog for a project device

    This QDialog shows the `Hash` configuration in a html format and can
    show the conflicts on request
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

        self.ui_show.clicked.connect(self.check_button_clicked)
        self.project_device = project_device
        self.configuration = configuration
        self._show_config = True
        self.show_configuration()

    @Slot()
    def check_button_clicked(self):
        self._show_config = not self._show_config
        text = ("Check Configuration" if self._show_config
                else "Show Configuration")
        self.ui_show.setText(text)
        self.show_configuration()

    def show_configuration(self):
        """Show the desired configuration, which can be either the existing
         `configuration` or the checked `conflicts` configuration.

         The conflicts are visualized by creating a fresh binding from a
         schema and applying a configuration.
         """
        if self._show_config:
            config = self.configuration
            # Note: Protect as well against a `None` configuration!
            if config is None or config.empty():
                html = "<center>No changes in configuration!</center>"
            else:
                html = create_html_hash(config)
        else:
            # Get a conflict configuration
            proxy = self.project_device.get_class_proxy()
            binding = proxy.binding
            # No schema arrived yet!
            if not len(binding.value):
                html = "<center>No schema available!</center>"
            else:
                config = validate_binding_configuration(
                    binding=binding, config=self.configuration)
                if config.empty():
                    html = "<center>No conflicts in configuration!</center>"
                else:
                    html = create_html_hash(config)

        self.ui_text_info.setHtml(html)
        self.ui_text_info.adjustSize()
        scroll_bar = self.ui_text_info.verticalScrollBar()
        scroll_bar.setValue(scroll_bar.sliderPosition())
