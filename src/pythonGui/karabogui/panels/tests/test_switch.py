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


from qtpy.QtCore import QObject, Slot

from karabogui.testing import click_button

from ..switch import SwitchButton


class Mediator(QObject):
    state = False

    @Slot(bool)
    def receive(self, state):
        self.state = state


def test_switch_button(gui_app):
    mediator = Mediator()
    button = SwitchButton()
    button.toggled.connect(mediator.receive)
    click_button(button.button)
    assert mediator.state is True
    click_button(button.switch_frame)
    assert mediator.state is False
