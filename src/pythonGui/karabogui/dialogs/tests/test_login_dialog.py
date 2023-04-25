# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest import main

from karabogui.dialogs.api import LoginDialog
from karabogui.testing import GuiTestCase


class TestLoginDialog(GuiTestCase):

    def test_basic_dialog(self):
        dialog = LoginDialog()
        assert dialog.password == "karabo"
        assert dialog.username == ""
        assert dialog.hostname == "localhost"

        assert not dialog.isModal()

        dialog = LoginDialog(username="admin")
        assert dialog.username == "admin"

        dialog = LoginDialog(port=10000)
        assert dialog.port == 10000
        dialog = LoginDialog(port="10001")
        assert dialog.port == 10001

        dialog = LoginDialog(password="karabo3")
        assert dialog.password == "karabo3"

        dialog = LoginDialog(hostname="notlocal")
        assert dialog.hostname == "notlocal"


if __name__ == "__main__":
    main()
