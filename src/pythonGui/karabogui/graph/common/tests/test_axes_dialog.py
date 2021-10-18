from unittest import main, mock

from qtpy.QtWidgets import QDialog

from karabogui.graph.common.api import AxesLabelsDialog
from karabogui.testing import GuiTestCase


class TestDialogs(GuiTestCase):

    def test_view_dialog(self):
        # Test no config dialog
        empty_config = {"x_label": "",
                        "y_label": "",
                        "x_units": "",
                        "y_units": ""}

        dialog = AxesLabelsDialog()
        self.assertEqual(dialog.labels, empty_config)

        config = {"x_label": "Time",
                  "y_label": "#",
                  "x_units": "s",
                  "y_units": ""}
        dialog = AxesLabelsDialog(config=config)
        self.assertEqual(dialog.labels, config)

        path = "karabogui.graph.common.dialogs.axes_labels.AxesLabelsDialog"
        with mock.patch(path) as d:
            d().exec.return_value = QDialog.Accepted
            d().labels = dialog.labels
            # We fake a success return and test the boolean, content reply
            content, success = dialog.get(None, None)
            self.assertTrue(success)
            self.assertEqual(content, config)


if __name__ == "__main__":
    main()
