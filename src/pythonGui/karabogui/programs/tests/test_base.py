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
import os
import sys

from qtpy.QtCore import QLocale

from karabogui.const import IS_MAC_SYSTEM
from karabogui.programs.base import create_gui_app, init_gui
from karabogui.testing import set_test_organization_info


def test_start_app(mocker):
    """Make sure that we can start a gui application"""
    os.environ["KARABO_TEST_GUI"] = "1"
    info = mocker.patch("karabogui.programs.base.set_app_info")
    app = create_gui_app(sys.argv)
    info.assert_called_with(app, "XFEL", "xfel.eu", "KaraboGUI")
    init_gui(app, use_splash=True)
    set_test_organization_info(app)
    font = app.font()
    assert font.family() == "Source Sans Pro"
    psize = 10 if not IS_MAC_SYSTEM else 13
    assert font.pointSize() == psize
    locale = QLocale()
    assert locale.country() == QLocale.UnitedStates
    assert locale.language() == QLocale.English
