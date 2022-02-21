from unittest import main, mock

from qtpy.QtCore import QItemSelectionModel
from qtpy.QtWidgets import QApplication

from karabo.native import Hash
from karabogui.events import KaraboEvent
from karabogui.testing import GuiTestCase
from karabogui.widgets.log import LogWidget


def create_log_data():
    logs = []

    for i in range(100):
        gui_server = Hash(
            "timestamp", "2021-07-05T15:08:35.250",
            "type", "INFO",
            "category", f"Karabo_GuiServer_{i}",
            "message", "Hello!")
        logs.append(gui_server)

        debug_log = Hash(
            "timestamp", "2021-07-05T13:09:25.120",
            "type", "DEBUG",
            "category", f"XFEL_EG_RACK/MOTOR/{i}",
            "message", "Schema Updated")
        logs.append(debug_log)

        error_log = Hash(
            "timestamp", "2021-07-05T13:09:25.120",
            "type", "ERROR",
            "category", f"XFEL_EG_RACK/MOTOR/{i}",
            "message", "Error in background task",
            "traceback", "This is a traceback"
        )
        logs.append(error_log)

    return logs


class TestLogWidget(GuiTestCase):

    def test_log_widget_prune(self):
        widget = LogWidget()
        data = create_log_data()
        widget.onLogDataAvailable(data)
        self.assertEqual(widget.tailindex, 300)
        # add 300 more step by step
        widget.onLogDataAvailable(data)
        self.assertEqual(widget.tailindex, 600)
        widget.onLogDataAvailable(data)
        self.assertEqual(widget.tailindex, 900)
        # add 300 more and get pruned down
        widget.onLogDataAvailable(data)
        self.assertEqual(widget.tailindex, 200)

        widget.onLogDataAvailable(data)
        self.assertEqual(widget.tailindex, 500)
        widget.onClearLog()
        self.assertEqual(widget.logs, [])
        self.assertEqual(widget.tailindex, 0)

    def test_log_widget_filter(self):
        widget = LogWidget()
        data = create_log_data()
        widget.onLogDataAvailable(data)
        widget.onLogDataAvailable(data)
        self.assertEqual(widget.tailindex, 600)

        self.assertEqual(widget.queryModel.rowCount(None), 600)
        widget.onFilterChanged()
        self.assertEqual(widget.queryModel.rowCount(None), 600)

        self.assertEqual(widget.pbFilterOptions.text(),
                         "+ Show filter options")
        widget.onFilterOptionVisible(True)
        self.assertEqual(widget.pbFilterOptions.text(),
                         "- Hide filter options")

        widget.pbFilterDebug.setChecked(False)
        widget.onFilterChanged()
        self.assertEqual(widget.queryModel.rowCount(None), 400)
        widget.pbFilterError.setChecked(False)
        widget.onFilterChanged()
        self.assertEqual(widget.queryModel.rowCount(None), 200)
        widget.pbFilterInfo.setChecked(False)
        widget.onFilterChanged()
        self.assertEqual(widget.queryModel.rowCount(None), 0)
        self.assertEqual(widget.tailindex, 600)

        widget.pbFilterDebug.setChecked(True)
        widget.pbFilterError.setChecked(True)
        widget.pbFilterInfo.setChecked(True)
        widget.onFilterChanged()
        self.assertEqual(widget.queryModel.rowCount(None), 600)
        self.assertEqual(widget.tailindex, 600)

        # default search is description
        widget.leSearch.setText("XFEL_EG_RACK/MOTOR/95")
        widget.onFilterChanged()
        self.assertEqual(widget.queryModel.rowCount(None), 0)

        # Must activate search before for instanceId
        widget.pbSearchInsId.setChecked(True)
        widget.onFilterChanged()
        # Must be 4, as two times defaults stacked up
        self.assertEqual(widget.queryModel.rowCount(None), 4)

        # Change filter option back
        widget.onFilterOptionVisible(False)
        self.assertEqual(widget.pbFilterOptions.text(),
                         "+ Show filter options")

    def test_clipboard_doubleclick(self):
        widget = LogWidget()
        data = create_log_data()
        widget.onLogDataAvailable(data)
        path = "karabogui.widgets.log.broadcast_event"
        with mock.patch(path) as broadcast:
            index = widget.queryModel.createIndex(12, 0)
            widget.onItemDoubleClicked(index)
            broadcast.assert_called_with(KaraboEvent.ShowDevice,
                                         {"deviceId": "XFEL_EG_RACK/MOTOR/95"})

        selection_model = widget.table.selectionModel()
        selection_model.setCurrentIndex(index,
                                        QItemSelectionModel.ClearAndSelect)

        widget._copy_clipboard()
        clipboard = QApplication.clipboard()
        text = clipboard.text()
        self.assertIn("XFEL_EG_RACK/MOTOR/95", text)


if __name__ == "__main__":
    main()
