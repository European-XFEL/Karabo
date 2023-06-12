#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on June 3, 2021
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
from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import QDialog, QDialogButtonBox

import karabogui.icons as icons
from karabo.native import Hash, create_html_hash, has_changes

from .utils import get_dialog_ui


def config_changes(a, b):
    """Compare two `Hash` configurations `a` and `b`"""
    changes = Hash()
    for key, a_value, _ in Hash.flat_iterall(a):
        if key not in b:
            changes[key] = "Value was removed ..."
            continue
        b_value = b[key]
        if has_changes(a_value, b_value):
            changes[key] = b_value
    for key, b_value, _ in Hash.flat_iterall(b):
        if key not in a:
            changes[key] = b_value

    return changes


class ConfigComparisonDialog(QDialog):
    """A simple configuration comparison dialog

    :param title: The title of the dialog
    :param old: The existing `old` configuration for comparison
    :param new: The `new` configuration for comparison
    :param parent: The dialog parent
    """

    def __init__(self, title, old, new, parent=None):
        super().__init__(parent=parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        uic.loadUi(get_dialog_ui("configuration_comparison.ui"), self)
        self.setWindowTitle(title)
        flags = Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint
        self.setWindowFlags(self.windowFlags() | flags)

        text = ("Note: The configuration ONLY reflects property "
                "values that are different from the device default.")
        self.ui_info.setText(text)

        ok_button = self.ui_buttonBox.button(QDialogButtonBox.Ok)
        ok_button.clicked.connect(self.accept)

        cancel_button = self.ui_buttonBox.button(QDialogButtonBox.Cancel)
        cancel_button.clicked.connect(self.reject)

        self._show_comparison = False
        self.ui_swap.setIcon(icons.change)
        self.ui_swap.clicked.connect(self._swap_view)
        ok_button = self.ui_buttonBox.button(QDialogButtonBox.Ok)
        ok_button.clicked.connect(self.accept)

        cancel_button = self.ui_buttonBox.button(QDialogButtonBox.Cancel)
        cancel_button.clicked.connect(self.reject)

        html_a = create_html_hash(old)
        html_b = create_html_hash(new)
        self.ui_config_existing.setHtml(html_a)
        self.ui_config_new.setHtml(html_b)

        changes = config_changes(old, new)
        self.ui_changes.setHtml(create_html_hash(changes))

    # ---------------------------------------------------------------------
    # Slot Interface

    @Slot()
    def _swap_view(self):
        self._show_comparison = not self._show_comparison
        text = ("Show Configuration" if self._show_comparison
                else "Show Comparison")
        self.ui_swap.setText(text)
        self.ui_stack_widget.setCurrentIndex(int(self._show_comparison))
