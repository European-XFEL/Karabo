# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
