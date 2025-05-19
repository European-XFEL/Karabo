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
import json
from os import path as op

from karabogui.dialogs import update_dialog
from karabogui.dialogs.update_dialog import (
    _PKG_NAME, UNDEFINED_VERSION, UpdateDialog, UpdateNoticeDialog,
    get_index_list, get_pkg_version)

_PKG = "GUIExtensions"


class MockReturn:
    def __init__(self, info: dict):
        self._info = info

    def json(self) -> dict:
        return self._info


def update_pkg(package_name: str):
    if package_name == _PKG:
        return "1.0.0"
    return UNDEFINED_VERSION


def test_update_dialog(gui_app, mocker):
    """Tests the model of the extension updater"""
    mocker.patch.object(update_dialog, "get_pkg_version",
                        side_effect=update_pkg)
    request = mocker.patch("requests.get")
    request.return_value = MockReturn({})
    dialog = UpdateDialog()
    assert dialog.label_current.text() == "1.0.0"
    assert dialog.label_latest.text() == UNDEFINED_VERSION


def test_dialog_index_parsing(gui_app, mocker):
    """Test the proper parsing of a json file from the index"""
    json_file = op.join(op.dirname(__file__), "test_json.json")
    with open(json_file) as f:
        package = json.loads(f.read())

    mocker.patch("requests.get", return_value=MockReturn(package))
    packages = update_dialog.get_index_list()
    assert packages.get(_PKG) == "0.0.1"


def test_dialog(gui_app):
    dialog = UpdateNoticeDialog()
    current_version = get_pkg_version(_PKG_NAME)
    latest_version = get_index_list().get(_PKG_NAME, "0.0.0")

    expected = (
        f"A newer version of {_PKG_NAME} available: <b>{latest_version}"
        f"</b>.<br>Current version is <b>{current_version}</b>.<br> "
        f"<br>Would you like to install using the Update Dialog? <br> "
        f"<br> You can also update using the command-line utility: "
        f"<br><i>karabo-update-extensions -l </i>")
    assert dialog.info_label.text() == expected
