# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
        model = widget.table.model()
        data = create_log_data()
        widget.onLogDataAvailable(data)
        self.assertEqual(model.rowCount(), 300)
        # add 300 more to overshoot and prune, stays at 300
        widget.onLogDataAvailable(data)
        self.assertEqual(model.rowCount(), 300)
        self.assertGreater(model.rowCount(), 0)
        widget.onClearLog()
        self.assertEqual(model.rowCount(), 0)
        # Do a reset with new data
        widget.initialize(data)
        self.assertEqual(model.rowCount(), 300)

    def test_clipboard_doubleclick(self):
        widget = LogWidget()
        data = create_log_data()
        widget.onLogDataAvailable(data)
        path = "karabogui.widgets.log.broadcast_event"
        with mock.patch(path) as broadcast:
            source_index = widget.table_model.createIndex(12, 0)
            index = widget.filter_model.mapFromSource(source_index)
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
