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

from karabogui.dialogs.api import FormatLabelDialog
from karabogui.testing import GuiTestCase


class TestFormatLabelDialog(GuiTestCase):

    def test_basic_dialog(self):
        dialog = FormatLabelDialog()
        assert dialog.font_size == 10
        assert dialog.font_weight == "normal"

        assert dialog.font_weight_combobox.count() == 2
        assert dialog.font_size_combobox.count() == 17

        dialog = FormatLabelDialog(font_size=12)
        assert dialog.font_size == 12


if __name__ == "__main__":
    main()
