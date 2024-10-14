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

from karabo.native import Hash
from karabogui.dialogs.api import ConfigComparisonDialog
from karabogui.testing import click_button


def test_config_dialog_basics(gui_app):
    old = Hash("float", 1.0)
    new = Hash("float", 1.2)
    dialog = ConfigComparisonDialog("Compare", old, new, None)

    assert not dialog._show_comparison
    assert dialog.ui_swap.text() == "Show Comparison"
    assert dialog.ui_config_existing.toPlainText() == "\nfloat\n1.0\n"
    assert dialog.ui_config_new.toPlainText() == "\nfloat\n1.2\n"

    click_button(dialog.ui_swap)
    assert dialog.ui_swap.text() == "Show Configuration"
    assert dialog._show_comparison
    # The value changed as well
    assert dialog.ui_changes.toPlainText() == "\nfloat\n1.2\n"

    # New dialog config
    old = Hash("float", 1.0, "boolProperty", False)
    new = Hash("float", 1.2)
    dialog = ConfigComparisonDialog("Compare", old, new, None)
    assert dialog.ui_changes.toPlainText() == (
        "\nfloat\n1.2\nboolProperty\nValue was removed ...\n")
