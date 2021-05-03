#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on April 23, 2021
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from qtpy import uic
from qtpy.QtCore import Slot, Qt
from qtpy.QtWidgets import QDialogButtonBox, QDialog

from karabo.native import create_html_hash, Hash, is_equal, writeXML

from karabogui import icons
from karabogui import messagebox
from karabogui.binding.api import extract_configuration
from karabogui.util import getSaveFileName
from karabogui.singletons.api import get_config

from .utils import get_dialog_ui


def get_config_changes(reference, new):
    """Extract the changes of Hash ``new`` with respect to Hash ``reference``
    """
    changes_ref, changes_new = Hash(), Hash()

    for key, ref_value, _ in Hash.flat_iterall(reference):
        if key not in new:
            continue
        new_value = new[key]
        if not is_equal(ref_value, new_value):
            changes_ref[key] = ref_value
            changes_new[key] = new_value

    return changes_ref, changes_new


class ConfigPreviewDialog(QDialog):
    """A simple configuration preview for configuration from name and time

    :param title: The title of the dialog
    :param info: The text info for the field (sub title). If not info is
                 provided, the field is invisible
    :param proxy: The proxy instance, can be from project or topology
    :param parent: The dialog parent
    """

    def __init__(self, title, info, configuration, proxy, parent=None):
        super().__init__(parent=parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        ui_file = get_dialog_ui("configuration_preview.ui")
        uic.loadUi(ui_file, self)
        self.setWindowTitle(title)
        flags = Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint
        self.setWindowFlags(self.windowFlags() | flags)

        if info is None:
            self.ui_info.setVisible(False)
        else:
            self.ui_info.setText(info)

        self._show_changes = False
        self.ui_swap.setIcon(icons.change)
        self.ui_swap.clicked.connect(self._swap_view)
        ok_button = self.ui_buttonBox.button(QDialogButtonBox.Ok)
        ok_button.clicked.connect(self.accept)

        cancel_button = self.ui_buttonBox.button(QDialogButtonBox.Cancel)
        cancel_button.clicked.connect(self.reject)

        self.ui_save_configuration.clicked.connect(
            self.save_configuration_to_file)
        self.ui_save_configuration.setIcon(icons.save)

        self.proxy = proxy
        self.configuration = configuration
        self._show_configuration()
        self._show_configuration_changes()

    def _show_configuration_changes(self):
        if not len(self.proxy.binding.value):
            # Note: We are protected by the menu bar that does not allow us
            # to be launched in this case. However, we protect again here...
            text = "No schema available to extract a configuration"
            self.ui_existing.setHtml(text)
            text = "No schema available for comparison of configurations!"
            self.ui_retrieved.setHtml(text)
            return

        existing = extract_configuration(self.proxy.binding)
        changes_a, changes_b = get_config_changes(existing, self.configuration)
        html_a = create_html_hash(changes_a, include_attributes=False)
        html_b = create_html_hash(changes_b, include_attributes=False)

        self.ui_existing.setHtml(html_a)
        self.ui_retrieved.setHtml(html_b)

    def _show_configuration(self):
        html = create_html_hash(self.configuration, include_attributes=False)
        self.ui_text_info.setHtml(html)

    # ---------------------------------------------------------------------
    # Slot Interface

    @Slot()
    def _swap_view(self):
        self._show_changes = not self._show_changes
        text = "Show Configuration" if self._show_changes else "Show Changes"
        self.ui_swap.setText(text)
        self.ui_stack_widget.setCurrentIndex(int(self._show_changes))

        text = "Changes" if self._show_changes else "Retrieved Configuration"
        self.ui_show.setText(text)

    @Slot()
    def save_configuration_to_file(self):
        if self.configuration.empty():
            messagebox.show_error("The configuration is empty and cannot "
                                  "be saved", title='No configuration',
                                  parent=self)
            return

        config = get_config()
        path = config['config_dir']
        directory = path if path and op.isdir(path) else ""

        class_id = self.configuration.get("classId", "unknown-class")

        # Use deviceId for the name!
        default_name = self.configuration.get('deviceId')
        if default_name:
            default_name = default_name.replace('/', '-') + '.xml'
        else:
            default_name = class_id + '.xml'

        filename = getSaveFileName(caption="Save configuration as",
                                   filter="Configuration (*.xml)",
                                   suffix="xml",
                                   directory=directory,
                                   selectFile=default_name,
                                   parent=self)
        if not filename:
            return

        config = Hash(class_id, self.configuration)

        # Save configuration to file
        with open(filename, 'w') as fp:
            writeXML(config, fp)

        # save the last config directory
        get_config()['config_dir'] = op.dirname(filename)
