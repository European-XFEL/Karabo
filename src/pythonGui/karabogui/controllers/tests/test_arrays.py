import numpy as np

from karabo.native import Configurable, Hash, NDArray, UInt32

from karabogui.testing import (
    get_class_property_proxy, set_proxy_hash)

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
    property_value = get_array_data(proxy)
    np.testing.assert_almost_equal(property_value, value)

    schema = TwoObject.getClassSchema()
    proxy = get_class_property_proxy(schema, 'prop')
    value = np.array([[np.random.randint(0, 50) for _ in range(10)]
                      for _ in range(2)], dtype="uint32")
    array_hash = Hash('type', 14,
                      'data', value.tobytes())
    h = Hash('prop', array_hash)
    set_proxy_hash(proxy, h)
    property_value = get_array_data(proxy)
    # XXX: Traits does casting to a different 1 dimensional shape!
    assert property_value is not None
