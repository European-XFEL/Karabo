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
