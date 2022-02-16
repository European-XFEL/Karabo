from unittest import main, mock

from qtpy.QtCore import QItemSelectionModel, Qt
from qtpy.QtGui import QClipboard
from qtpy.QtWidgets import QApplication

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
            self.assertEqual(dialog.server_id, server_id)
            self.assertEqual(dialog.ui_server_id.text(), server_id)

            logs = Hash("content", data)
            dialog.request_handler(True, logs)
            table_view = dialog.table
            table_model = table_view.model()
            self.assertEqual(table_model.rowCount(), 2)

            # Get instanceId
            index = table_model.index(0, 2)
            actual_instanceId = "XHQ_EG_DG/MOTOR/MOTOR1"
            instanceId = table_model.data(index, role=Qt.DisplayRole)
            self.assertEqual(instanceId, actual_instanceId)

            # Select that index
            table_view.selectionModel().setCurrentIndex(
                index, QItemSelectionModel.ClearAndSelect)
            path = "karabogui.dialogs.log_dialog.broadcast_event"
            with mock.patch(path) as broad:
                dialog.onItemDoubleClicked(index)
                broad.assert_called_with(KaraboEvent.ShowDevice,
                                         {"deviceId": actual_instanceId})

            # Clipboard
            dialog._copy_clipboard()
            clipboard = QApplication.clipboard()
            text = clipboard.text(mode=QClipboard.Clipboard)
            self.assertIn(actual_instanceId, text)
            self.assertIn("INFO", text)


if __name__ == "__main__":
    main()
