# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
