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
from unittest import TestCase

from qtpy.QtGui import QValidator

from ..webdialog import WebValidator


class TestWebValidator(TestCase):

    def setUp(self):
        self._validator = WebValidator()

    def test_valid_url(self):
        user_pass_url = "http://username:password@someurl.com"
        ip_address = "192.168.1.1"
        localhost_with_port = "localhost:8080"
        simple_website = "google.com"
        query_url = "ftp://someurl.com/page.html?param1=1&param2=2"
        cinema_url = "karabo://cinema?param1=1&param2=2"
        new_gui_url = "karabo://gui"

        self._assert_url(user_pass_url, valid=True)
        self._assert_url(ip_address, valid=True)
        self._assert_url(localhost_with_port, valid=True)
        self._assert_url(simple_website, valid=True)
        self._assert_url(query_url, valid=True)
        self._assert_url(new_gui_url, valid=True)
        self._assert_url(cinema_url, valid=True)
        self._assert_url("invalid url", valid=False)

    def _assert_url(self, url, *, valid):
        result, _, _ = self._validator.validate(url, pos=0)

        if valid:
            self.assertEqual(result, QValidator.Acceptable)
        else:
            self.assertNotEqual(result, QValidator.Acceptable)
