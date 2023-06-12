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
from karabogui.dialogs.api import DataViewDialog


def test_data_view(gui_app):
    data = Hash("Data", "Karabo")

    title = "This is the title"
    info = "This is the description info"
    dialog = DataViewDialog(title, info, data)
    assert dialog.windowTitle() == title
    assert dialog.ui_info.text() == info
    assert dialog.ui_text_info.toPlainText() == "\nData\nKarabo\n"
