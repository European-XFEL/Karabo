# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
