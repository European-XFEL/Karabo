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

import pytest
from qtpy.QtWidgets import QApplication

from karabo.native import AccessLevel
from karabogui.background import create_background_timer
from karabogui.controllers.api import populate_controller_registry


@pytest.fixture(scope="package")
def gui_app():
    os.environ["KARABO_TEST_GUI"] = "1"
    app = QApplication.instance()
    if app is None:
        app = QApplication(sys.argv)
    # Check organization for QSettings for tests, see the setting in
    # `programs/tests/test_base.py`
    app.setOrganizationName("TestFacility")
    app.setOrganizationDomain("karabo.test.eu")
    app.setApplicationName("pytest")
    assert app.applicationName() == "pytest"
    create_background_timer()
    import karabogui.access as krb_access
    krb_access.GLOBAL_ACCESS_LEVEL = AccessLevel.OPERATOR
    from karabogui import icons
    icons.init()
    populate_controller_registry()
    return app
