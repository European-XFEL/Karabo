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
from karabogui.testing import GuiTestCase
from karabogui.widgets.popup import PopupWidget


class TestPopUp(GuiTestCase):

    def test_default_widget(self):
        widget = PopupWidget()
        # Freeze is `False` as default
        layout = widget.layout()
        assert not layout.isEmpty()
        assert getattr(widget, '_ui_freeze_button', None) is None
        assert widget.freeze is False

        # Test the info setting
        info = {"minInc": 5, "description": "This is a description"}
        widget.setInfo(info)
        plain_text = "\nminInc: \n5\ndescription: \nThis is a description\n"
        assert widget.text == plain_text

        info = Hash("minInc", 5, "description", "This is a description")
        widget.setInfo(info)
        # Hashes are different
        plain_text = "\nminInc\n5\ndescription\nThis is a description\n"
        assert widget.text == plain_text
        widget.close()

    def test_freeze_widget(self):
        widget = PopupWidget(can_freeze=True)
        layout = widget.layout()
        assert not layout.isEmpty()
        assert getattr(widget, '_ui_freeze_button', None) is not None
        assert widget.freeze is False

        # Test the info setting
        info = {"minInc": 20, "description": "This is freeze"}
        widget.setInfo(info)
        plain_text = "\nminInc: \n20\ndescription: \nThis is freeze\n"
        assert widget.text == plain_text

        # Toggle freeze and test the caching
        widget.toggle_freeze()
        assert widget.freeze is True
        # Test the info setting
        info = {"alarmLow": 20, "displayedName": "Alarm Info"}
        widget.setInfo(info)
        # Text did not change... But if we toggle, we change again!
        assert widget.text == plain_text
        widget.toggle_freeze()
        assert widget.freeze is False
        plain_text = "\nalarmLow: \n20\ndisplayedName: \nAlarm Info\n"
        assert widget.text == plain_text

        # Test the reset
        widget.toggle_freeze()
        assert widget.freeze is True
        widget.reset()
        assert widget.freeze is False
        assert widget.text == plain_text
        widget.close()
