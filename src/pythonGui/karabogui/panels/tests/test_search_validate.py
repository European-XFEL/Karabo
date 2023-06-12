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

from ..tool_widget import SearchValidator

LONG_NAME = "middlelayerServer/veryveryveryveryveryverylongnameforaserver" \
            "thatshouldnotbethere"


def test_validator():
    """Test the search validator for the navigation panels"""
    validator = SearchValidator()

    # Empty string must always be `ok`
    valid, *_ = validator.validate("", None)
    assert valid is QValidator.Acceptable

    valid, *_ = validator.validate("aAbBC", None)
    assert valid is QValidator.Acceptable

    # White space does not work
    valid, *_ = validator.validate(" ", None)
    assert valid is QValidator.Invalid

    valid, *_ = validator.validate("aBa ", None)
    assert valid is QValidator.Invalid

    # Normal devices and servers

    valid, *_ = validator.validate("SA4_RR_SYS/PLC/RACK-9", None)
    assert valid is QValidator.Acceptable

    valid, *_ = validator.validate("SA4_RR_SYS/PLC/RACK-9/BROKEN", None)
    assert valid is QValidator.Acceptable

    valid, *_ = validator.validate("cppServer/beckhoff_mc2", None)
    assert valid is QValidator.Acceptable

    valid, *_ = validator.validate("////", None)
    assert valid is QValidator.Acceptable

    valid, *_ = validator.validate("////---//", None)
    assert valid is QValidator.Acceptable

    valid, *_ = validator.validate(LONG_NAME, None)
    assert valid is QValidator.Acceptable

    valid, *_ = validator.validate("#", None)
    assert valid is QValidator.Invalid

    valid, *_ = validator.validate(":", None)
    assert valid is QValidator.Invalid
