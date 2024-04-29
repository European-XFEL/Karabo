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
import sys
from os import path as op
from unittest import mock, skip

from karabogui.dialogs import update_dialog
from karabogui.dialogs.update_dialog import UNDEFINED_VERSION
from karabogui.testing import GuiTestCase


class TestCase(GuiTestCase):

    def _get_loaded_modules(self):
        extensions = []
        for module in sys.modules:
            if module.startswith("extensions"):
                extensions.append(module)
        return extensions

    @skip(reason="Install is not working properly in CI")
    def test_current_version_bug(self):
        """Bug found when updating a package version. After the
        package is removed pkg_resources still thinks it"s there."""
        update_dialog.uninstall_package()
        assert update_dialog.get_current_version() == UNDEFINED_VERSION
        assert self._get_loaded_modules() == []

        update_dialog.install_package("0.1.0")
        assert update_dialog.get_current_version() == "0.1.0"
        assert self._get_loaded_modules() == [
            "extensions", "extensions.display_ipm_quadrant",
            "extensions.models", "extensions.models.simple"]

        update_dialog.uninstall_package()
        assert update_dialog.get_current_version() == UNDEFINED_VERSION
        assert self._get_loaded_modules() == []

    def test_extensions_dialog(self):
        mock_obj = mock.patch.object
        m1 = mock_obj(update_dialog, "get_current_version", return_value="1")
        m2 = mock_obj(update_dialog, "get_latest_version", return_value="1")

        with m1, m2:
            dialog = update_dialog.UpdateDialog()

            assert not dialog.button_update.isEnabled()
            assert dialog.label_current.text() == "1"
            assert dialog.label_latest.text() == "1"

            update_dialog.get_latest_version.return_value = "2"

            self.click(dialog.button_refresh)

            # Should refresh current and latest versions
            assert dialog.label_current.text() == "1"
            assert dialog.label_latest.text() == "2"

            assert dialog.button_update.isEnabled()

    def test_latest_version(self):
        """Tests the model of the extension updater"""
        json_file = op.join(op.dirname(__file__), "test_json.json")
        with open(json_file) as f:
            packages = json.loads(f.read())

        with mock.patch.object(update_dialog,
                               "_retrieve_package_list",
                               return_value=packages):
            latest_version = update_dialog.get_latest_version()
            assert latest_version == "0.0.1"
