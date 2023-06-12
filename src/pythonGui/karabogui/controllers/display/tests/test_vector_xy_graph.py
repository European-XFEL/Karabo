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

from karabo.native import Configurable, VectorInt32
from karabogui.binding.api import PropertyProxy
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..vector_xy_graph import DisplayVectorXYGraph


class Object(Configurable):
    index = VectorInt32()
    value = VectorInt32()


def test_vector_xy_graph(gui_app):
    """Test the setting of both x and y values in the vector xy graph"""
    # setup
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "index")
    device = proxy.root_proxy
    value_proxy = PropertyProxy(root_proxy=device, path="value")

    controller = DisplayVectorXYGraph(proxy=proxy)
    controller.create(None)
    assert controller.widget is not None
    controller.visualize_additional_property(value_proxy)

    # set values
    index = [1, 2, 3, 4, 5]
    value = [42] * 5
    set_proxy_value(proxy, "index", index)
    set_proxy_value(value_proxy, "value", value)

    curve = controller._curves[value_proxy]
    np.testing.assert_almost_equal(list(curve.xData), index)
    np.testing.assert_almost_equal(list(curve.yData), value)

    # teardown
    controller.destroy()
    assert controller.widget is None
