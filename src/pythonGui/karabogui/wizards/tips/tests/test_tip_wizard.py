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
        assert len(self.wizard.pageIds()) == 8
        assert self.wizard.windowTitle() == "Karabo Tips & Tricks"
        assert self.wizard.wizardStyle() == QWizard.ClassicStyle

        assert self.wizard.startId() == 0

        assert self.wizard.currentId() == -1
        self.wizard.restart()
        assert self.wizard.currentId() == 0
        self.wizard.next()
        assert self.wizard.currentId() == 1

        # Test the deactivation for next time
        config = Configuration()
        with singletons(configuration=config):
            self.wizard.update_start(0)
            assert config["wizard"] is True
            self.wizard.update_start(1)
            assert config["wizard"] is False

    def test_button_object_names(self):
        next_button = self.wizard.button(QWizard.NextButton)
        assert next_button.objectName() == "NextButton"
        back_button = self.wizard.button(QWizard.BackButton)
        assert back_button.objectName() == "BackButton"


if __name__ == "__main__":
    main()
