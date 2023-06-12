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
from karabo.native import Configurable, Int32, MetricPrefix, Unit
from karabogui.testing import get_class_property_proxy

from ..util import axis_label


class Object(Configurable):
    prop = Int32(displayedName='Hello',
                 unitSymbol=Unit.METER,
                 metricPrefixSymbol=MetricPrefix.KILO)


def test_axis_label():
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, 'prop')
    assert axis_label(proxy) == 'Hello [km]'
