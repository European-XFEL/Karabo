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
