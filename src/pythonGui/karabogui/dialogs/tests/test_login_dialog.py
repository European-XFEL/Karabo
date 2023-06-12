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
