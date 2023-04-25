# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from qtpy.QtWidgets import QDialog

from karabogui.graph.common.api import AxesLabelsDialog


def test_view_dialog(gui_app, mocker):
    # Test no config dialog
    empty_config = {"x_label": "",
                    "y_label": "",
                    "x_units": "",
                    "y_units": ""}

    dialog = AxesLabelsDialog()
    assert dialog.windowTitle() == "Edit Axes Details"
    assert dialog.labels == empty_config

    config = {"x_label": "Time",
              "y_label": "#",
              "x_units": "s",
              "y_units": ""}
    dialog = AxesLabelsDialog(config=config)
    assert dialog.labels == config

    path = "karabogui.graph.common.dialogs.axes_labels.AxesLabelsDialog"
    d = mocker.patch(path)
    d().exec.return_value = QDialog.Accepted
    d().labels = dialog.labels
    # We fake a success return and test the boolean, content reply
    content, success = dialog.get(None, None)
    assert success
    assert content == config
