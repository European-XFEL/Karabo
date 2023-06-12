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
from unittest import mock

from karabogui.testing import GuiTestCase
from karabogui.widgets.find_toolbar import FindToolBar


class TestFindToolBar(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.find_toolbar = FindToolBar()
        self.find_toolbar.find_line_edit.setText("foo")

    def test_find(self):
        """Find should emit the 'findRequest' signal with correct arguments"""
        mock_slot = mock.Mock()
        self.find_toolbar.findRequested.connect(mock_slot)

        # Find Next
        self.find_toolbar.findNext()
        assert mock_slot.call_count == 1
        call_args = mock_slot.call_args[0]
        assert call_args == ("foo", False, False)

        # Find Previous
        mock_slot.reset_mock()
        self.find_toolbar.findPrevious()
        assert mock_slot.call_count == 1
        call_args = mock_slot.call_args[0]
        assert call_args == ("foo", False, True)

        # Find Next case-sensitive
        self.find_toolbar.match_case.setChecked(True)
        mock_slot.reset_mock()
        self.find_toolbar.findNext()
        assert mock_slot.call_count == 1
        call_args = mock_slot.call_args[0]
        assert call_args == ("foo", True, False)

    def test_setResultText(self):
        """Test the label gets updated with the correct text"""
        self.find_toolbar.setResultText(0)
        assert self.find_toolbar.result_label.text() == "0 Results"
        self.find_toolbar.setResultText(10)
        assert self.find_toolbar.result_label.text() == "10 Results"

        self.find_toolbar.setResultText(1)
        assert self.find_toolbar.result_label.text() == "1 Result"
