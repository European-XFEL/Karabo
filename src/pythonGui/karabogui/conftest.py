# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os
import sys
from platform import system

import pytest
from qtpy.QtWidgets import QApplication

from karabo.native import AccessLevel
from karabogui.background import create_background_timer
from karabogui.controllers.api import populate_controller_registry


@pytest.fixture(scope="package")
def gui_app():
    os.environ["KARABO_TEST_GUI"] = "1"
    if system() == 'Darwin' and 'QT_MAC_WANTS_LAYER' not in os.environ:
        os.environ['QT_MAC_WANTS_LAYER'] = '1'
    app = QApplication.instance()
    if app is None:
        app = QApplication(sys.argv)
    create_background_timer()
    import karabogui.access as krb_access
    krb_access.GLOBAL_ACCESS_LEVEL = AccessLevel.OPERATOR
    from karabogui import icons
    icons.init()
    populate_controller_registry()
    return app
