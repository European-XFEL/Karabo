# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest import main

from qtpy.QtWidgets import QWizard

from karabogui.singletons.configuration import Configuration
from karabogui.testing import GuiTestCase, singletons

from ..wizard import TipsTricksWizard


class TestTipWizard(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.wizard = TipsTricksWizard()

    def tearDown(self):
        super().tearDown()
        self.wizard = None

    def test_basic_wizard(self):
        self.assertEqual(len(self.wizard.pageIds()), 8)
        self.assertEqual(self.wizard.windowTitle(), "Karabo Tips & Tricks")
        self.assertEqual(self.wizard.wizardStyle(), QWizard.ClassicStyle)

        self.assertEqual(self.wizard.startId(), 0)

        self.assertEqual(self.wizard.currentId(), -1)
        self.wizard.restart()
        self.assertEqual(self.wizard.currentId(), 0)
        self.wizard.next()
        self.assertEqual(self.wizard.currentId(), 1)

        # Test the deactivation for next time
        config = Configuration()
        with singletons(configuration=config):
            self.wizard.update_start(0)
            self.assertEqual(config["wizard"], True)
            self.wizard.update_start(1)
            self.assertEqual(config["wizard"], False)


if __name__ == "__main__":
    main()
