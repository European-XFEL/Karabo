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

from qtpy.QtCore import QSize

from karabogui.testing import GuiTestCase
from karabogui.widgets.toolbar import ToolBar


class TestPopUp(GuiTestCase):

    def test_default(self):
        widget = ToolBar()
        assert widget.windowTitle() == ""
        assert widget.iconSize() == QSize(19, 19)
        assert widget.isMovable() is False
        assert widget.isFloatable() is True

        # title test
        widget = ToolBar("new toolbar")
        assert widget.windowTitle() == "new toolbar"

        with mock.patch.object(widget, 'addWidget') as mock_method:
            # add_expander will add a widget to the toolbar. This way
            # we can test if that method was called.
            mock_method.assert_not_called()
            widget.add_expander()
            mock_method.assert_called_once()
