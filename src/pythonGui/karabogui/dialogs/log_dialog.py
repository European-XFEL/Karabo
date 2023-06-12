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
from qtpy import uic
from qtpy.QtCore import QSize, Qt, Slot
from qtpy.QtWidgets import QDialog

from karabo.common.api import WeakMethodRef
from karabogui import messagebox
from karabogui.events import (
    KaraboEvent, register_for_broadcasts, unregister_from_broadcasts)
from karabogui.request import call_device_slot
from karabogui.util import get_spin_widget
from karabogui.widgets.log import LogWidget

from .utils import get_dialog_ui


class LogDialog(QDialog):
    def __init__(self, server_id, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui("log_dialog.ui"), self)
        flags = Qt.WindowCloseButtonHint | Qt.WindowTitleHint
        self.setWindowFlags(self.windowFlags() | flags)

        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        self.setSizeGripEnabled(True)
        self.setWindowTitle(f"Server log for serverId: {server_id}")
        self.log_widget = LogWidget(resize_contents=True, parent=self)
        self.log_layout.insertWidget(0, self.log_widget)

        self.server_id = server_id
        self.ui_server_id.setText(server_id)
        self.spin_widget = get_spin_widget(icon="wait-black",
                                           scaled_size=QSize(16, 16))
        self.spin_widget.setVisible(False)
        self.bottom_layout.insertWidget(0, self.spin_widget)
        self.ui_request.clicked.connect(self.request_logger_data)
        self.ui_fetching_data.setVisible(False)
        self.request_logger_data()
        self.event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network
        }
        register_for_broadcasts(self.event_map)

    def _event_network(self, data):
        if not data.get("status"):
            self.close()

    def done(self, result):
        """Stop listening for broadcast events"""
        unregister_from_broadcasts(self.event_map)
        super().done(result)

    def request_handler(self, success, reply):
        self.spin_widget.setVisible(False)
        self.ui_fetching_data.setVisible(False)
        if not success:
            messagebox.show_error(
                f"Could not fetch the logs of server <b>{self.server_id}</b>.",
                parent=self)
            return

        data = reply["content"]
        self.log_widget.initialize(data)

    def keyPressEvent(self, event):
        if (event.key() in (Qt.Key_Enter, Qt.Key_Return) and
                self.focusWidget() == self.ui_number_logs):
            self.request_logger_data()
            event.accept()
            return
        return super().keyPressEvent(event)

    @Slot()
    def request_logger_data(self):
        self.spin_widget.setVisible(True)
        self.ui_fetching_data.setVisible(True)
        call_device_slot(WeakMethodRef(self.request_handler), self.server_id,
                         "slotLoggerContent",
                         logs=self.ui_number_logs.value())
