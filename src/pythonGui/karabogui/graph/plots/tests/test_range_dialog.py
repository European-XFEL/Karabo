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
from karabogui.graph.plots.api import RangeDialog


def test_range_dialog(gui_app):
    """Test the range dialog"""
    config = {
        "x_min": 0,
        "x_max": 20,
        "x_autorange": False,
    }
    actual = {
        "x_min": 2,
        "x_max": 3,
        "x_autorange": False,
    }
    dialog = RangeDialog(config, actual)

    assert dialog.ui_view_min.text() == "2.00e+00"
    assert dialog.ui_view_max.text() == "3.00e+00"

    assert dialog.ui_min.text() == "0"
    assert dialog.ui_max.text() == "20"
    assert dialog.ui_min.isEnabled()
    assert dialog.ui_max.isEnabled()
    assert not dialog.ui_autorange.isChecked()

    dialog.ui_autorange.setChecked(True)
    assert not dialog.ui_min.isEnabled()
    assert not dialog.ui_max.isEnabled()

    new = {
        "x_min": 0,
        "x_max": 20,
        "x_autorange": True}

    assert dialog.limits == new
