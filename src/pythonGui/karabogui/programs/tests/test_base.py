# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os
import sys
from unittest import TestCase

from qtpy.QtCore import QLocale

from karabogui.const import IS_MAC_SYSTEM
from karabogui.programs.base import create_gui_app, init_gui


class TestMainApplication(TestCase):

    def test_start_app(self):
        """Make sure that we can start a gui application"""
        os.environ["KARABO_TEST_GUI"] = "1"
        app = create_gui_app(sys.argv)
        init_gui(app, use_splash=True)
        self.assertEqual(app.organizationName(), "XFEL")
        self.assertEqual(app.organizationDomain(), "xfel.eu")
        self.assertEqual(app.applicationName(), "KaraboGUI")

        # Set a different organization name to not erase the settings in
        # other tests!
        app.setOrganizationName("NoXFEL")

        # Test standard settings
        font = app.font()
        self.assertEqual(font.family(), "Source Sans Pro")

        psize = 10 if not IS_MAC_SYSTEM else 13
        self.assertEqual(font.pointSize(), psize)

        locale = QLocale()
        self.assertEqual(locale.country(), QLocale.UnitedStates)
        self.assertEqual(locale.language(), QLocale.English)
