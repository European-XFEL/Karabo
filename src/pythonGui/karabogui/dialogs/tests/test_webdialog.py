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

from qtpy.QtGui import QValidator

from ..webdialog import WebValidator


def test_valid_url(gui_app):
    validator = WebValidator()
    user_pass_url = "http://username:password@someurl.com"
    ip_address = "192.168.1.1"
    localhost_with_port = "localhost:8080"
    simple_website = "google.com"
    query_url = "ftp://someurl.com/page.html?param1=1&param2=2"
    cinema_url = "karabo://cinema?param1=1&param2=2"
    new_gui_url = "karabo://gui"
    _assert_url(validator, user_pass_url, valid=True)
    _assert_url(validator, ip_address, valid=True)
    _assert_url(validator, localhost_with_port, valid=True)
    _assert_url(validator, simple_website, valid=True)
    _assert_url(validator, query_url, valid=True)
    _assert_url(validator, new_gui_url, valid=True)
    _assert_url(validator, cinema_url, valid=True)
    _assert_url(validator, "invalid url", valid=False)


def _assert_url(validator, url, *, valid):
    result, _, _ = validator.validate(url, pos=0)
    if valid:
        assert result == QValidator.Acceptable
    else:
        assert result != QValidator.Acceptable
