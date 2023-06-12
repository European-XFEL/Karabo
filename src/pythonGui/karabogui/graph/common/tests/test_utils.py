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
from ..utils import float_to_string


def test_float_string():
    assert float_to_string(3.002) == "3"
    assert float_to_string(3.002, precision=3) == "3.002"
    assert float_to_string(3, precision=3) == "3"
    assert float_to_string(1e9, precision=3) == "1000000000"
    assert float_to_string(-3.0023344) == "-3"
    assert float_to_string(-3.0023344, precision=3) == "-3.002"
    assert float_to_string(-3.0023344, precision=4) == "-3.0023"
