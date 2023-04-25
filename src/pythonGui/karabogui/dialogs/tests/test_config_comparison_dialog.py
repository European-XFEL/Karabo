# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest import main

from karabo.native import Hash
from karabogui.dialogs.api import ConfigComparisonDialog
from karabogui.testing import GuiTestCase


class TestConfigurationComparisonDialog(GuiTestCase):

    def test_config_dialog_basics(self):
        old = Hash("float", 1.0)
        new = Hash("float", 1.2)
        dialog = ConfigComparisonDialog("Compare", old, new, None)

        self.assertFalse(dialog._show_comparison)
        self.assertEqual(dialog.ui_swap.text(), "Show changes")
        self.assertEqual(dialog.ui_config_existing.toPlainText(),
                         "\nfloat\n1.0\n")
        self.assertEqual(dialog.ui_config_new.toPlainText(),
                         "\nfloat\n1.2\n")

        self.click(dialog.ui_swap)
        self.assertEqual(dialog.ui_swap.text(), "Show Configuration")
        self.assertTrue(dialog._show_comparison)
        # The value changed as well
        self.assertEqual(dialog.ui_changes.toPlainText(),
                         "\nfloat\n1.2\n")

        # New dialog config
        old = Hash("float", 1.0, "boolProperty", False)
        new = Hash("float", 1.2)
        dialog = ConfigComparisonDialog("Compare", old, new, None)
        self.assertEqual(dialog.ui_changes.toPlainText(),
                         "\nfloat\n1.2\nboolProperty\nValue was removed ...\n")


if __name__ == "__main__":
    main()
