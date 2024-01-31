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
from unittest import main, mock

from qtpy.QtCore import QItemSelectionModel, Qt

from karabo.native import Hash, Timestamp
from karabogui.dialogs.api import LogDialog
from karabogui.events import KaraboEvent
from karabogui.singletons.mediator import Mediator
from karabogui.testing import GuiTestCase, singletons

data = [
    Hash("timestamp", Timestamp().toLocal(),
         "category", "XHQ_EG_DG/MOTOR/MOTOR1",
         "type", "INFO",
         "traceback", "",
         "message", "Schema updated"),

    Hash("timestamp", Timestamp().toLocal(),
         "category", "XHQ_EG_DG/MOTOR/MOTOR1",
         "type", "ERROR",
         "traceback", "",
         "message", "Error limit switch")
]


class TestLoginDialog(GuiTestCase):

    def test_basic_dialog(self):
        """Test the basic of the log dialog"""
        mediator = Mediator()
        with singletons(mediator=mediator):
            server_id = "swerver"
            dialog = LogDialog(server_id, parent=None)
            assert dialog.server_id == server_id
            assert dialog.ui_server_id.text() == server_id

            logs = Hash("content", data)
            dialog.request_handler(True, logs)
            table_view = dialog.log_widget.table
            table_model = table_view.model()
            assert table_model.rowCount() == 2

            # Get instanceId
            index = table_model.index(0, 2)
            actual_instanceId = "XHQ_EG_DG/MOTOR/MOTOR1"
            instanceId = table_model.data(index, role=Qt.DisplayRole)
            assert instanceId == actual_instanceId

            # Select that index
            table_view.selectionModel().setCurrentIndex(
                index, QItemSelectionModel.ClearAndSelect)
            path = "karabogui.widgets.log.broadcast_event"
            with mock.patch(path) as broad:
                dialog.log_widget.onItemDoubleClicked(index)
                broad.assert_called_with(KaraboEvent.ShowDevice,
                                         {"deviceId": actual_instanceId})

            path = "karabogui.dialogs.log_dialog.call_device_slot"
            with mock.patch(path) as call:
                self.click(dialog.ui_request)
                call.assert_called_once()


if __name__ == "__main__":
    main()
