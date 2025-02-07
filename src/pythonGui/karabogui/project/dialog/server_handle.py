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
from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import QCompleter, QDialog, QDialogButtonBox

from karabogui.topology.api import getTopology
from karabogui.util import InputValidator

from .utils import get_dialog_ui


class ServerHandleDialog(QDialog):
    def __init__(self, model=None, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui("server_handle.ui"), self)

        servers = self.get_available_servers()
        self.ui_server_id.addItems(servers)
        completer = QCompleter(servers, parent=self)
        completer.setCaseSensitivity(False)
        completer.setCompletionMode(QCompleter.PopupCompletion)
        completer.setFilterMode(Qt.MatchContains)
        self.ui_server_id.setCompleter(completer)

        if model is None:
            title = "Add server"
        else:
            title = "Edit server"
            index = self.ui_server_id.findText(model.server_id)
            if index < 0:
                server_edit = self.ui_server_id.lineEdit()
                server_edit.setText(model.server_id)
            else:
                self.ui_server_id.setCurrentIndex(index)

        self.setWindowTitle(title)

        validator = InputValidator(object_type="server", parent=self)
        self.ui_server_id.setValidator(validator)

        self.ui_server_id.currentIndexChanged.connect(self._update_button_box)
        self.ui_server_id.editTextChanged.connect(self._update_button_box)
        self._update_button_box()

    def get_available_servers(self):
        """ Get all available servers of the `systemTopology`"""
        servers = list(getTopology()["server"])
        return sorted(servers)

    @Slot()
    def _update_button_box(self):
        """Only enable Ok button, if title and configuration is set
        """
        enabled = len(self.ui_server_id.currentText())
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

    @property
    def server_id(self):
        return self.ui_server_id.currentText()
