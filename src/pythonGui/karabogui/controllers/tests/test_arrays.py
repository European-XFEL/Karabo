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
import numpy as np

from karabo.native import Configurable, Hash, NDArray, UInt32, VectorDouble
from karabogui.testing import get_class_property_proxy, set_proxy_hash

from ..arrays import get_array_data


class Object(Configurable):
    prop = NDArray(
        displayedName="One dimensional array",
        dtype=UInt32,
        shape=(10,))


class TwoObject(Configurable):
    prop = NDArray(
        displayedName="Two dimensional array",
        dtype=UInt32,
        shape=(10, 2))


def test_array():
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, 'prop')
    value = np.array(list(range(10)), dtype="uint32")
    array_hash = Hash('type', 14,
                      'data', value.tobytes())
    h = Hash('prop', array_hash)
    set_proxy_hash(proxy, h)
    property_value, _ = get_array_data(proxy)
    np.testing.assert_almost_equal(property_value, value)

    schema = TwoObject.getClassSchema()
    proxy = get_class_property_proxy(schema, 'prop')
    value = np.array([[np.random.randint(0, 50) for _ in range(10)]
                      for _ in range(2)], dtype="uint32")
    array_hash = Hash('type', 14,
                      'data', value.tobytes())
    h = Hash('prop', array_hash)
    set_proxy_hash(proxy, h)
    property_value, _ = get_array_data(proxy)
    # XXX: Traits does casting to a different 1 dimensional shape!
    assert property_value is not None


def test_vector():
    """Test the `get_array` function with vectors"""

    class VectorObject(Configurable):
        prop = VectorDouble(
            displayedName="Vector Double")

    schema = VectorObject.getClassSchema()
    proxy = get_class_property_proxy(schema, 'prop')
    property_value, _ = get_array_data(proxy)
    assert property_value is None
    property_value, _ = get_array_data(proxy, default=[])
    assert property_value == []
    value = [1, 2, 3, 4]
    h = Hash('prop', value)
    set_proxy_hash(proxy, h)
    property_value, _ = get_array_data(proxy)
    np.testing.assert_almost_equal(property_value, value)
