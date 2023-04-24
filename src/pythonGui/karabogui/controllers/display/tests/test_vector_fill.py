import numpy as np

from karabo.native import Configurable, VectorInt32
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..vector_fill_graph import DisplayVectorFillGraph


class Object(Configurable):
    value = VectorInt32()


def test_vector_fill_basics(gui_app):
    # setup
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "value")
    controller = DisplayVectorFillGraph(proxy=proxy)
    controller.create(None)
    assert controller.widget is not None

    # test body
    value = [2, 4, 6]
    set_proxy_value(proxy, "value", value)
    curve = controller._plot
    assert curve is not None
    np.testing.assert_array_equal(curve.yData, value)

    # teardown
    controller.destroy()
    assert controller.widget is None
