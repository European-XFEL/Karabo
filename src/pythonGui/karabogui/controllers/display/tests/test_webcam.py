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
from karabogui.binding.builder import build_binding
from karabogui.binding.config import apply_configuration
from karabogui.binding.proxy import DeviceProxy, PropertyProxy
from karabogui.controllers.display.tests.image import (
    get_image_hash, get_pipeline_schema)
from karabogui.graph.common.const import AXIS_ITEMS

from ..webcam_graph import DisplayWebCamGraph


def test_webcam_graph_basics(gui_app):
    # setup
    schema = get_pipeline_schema()
    binding = build_binding(schema)
    root_proxy = DeviceProxy(binding=binding)

    output_proxy = PropertyProxy(root_proxy=root_proxy, path="output.data")
    img_proxy = PropertyProxy(root_proxy=root_proxy, path="output.data.image")
    controller = DisplayWebCamGraph(proxy=img_proxy)

    # test basics
    controller.create(None)
    assert controller.widget is not None

    image_hash = get_image_hash()
    apply_configuration(image_hash, output_proxy.binding)

    plotItem = controller._plot
    assert not plotItem.menuEnabled()

    image_shape = list(plotItem.imageItem.image.shape)
    assert image_shape == image_hash["image"]["dims"]

    # Assert axis are disabled
    for axis in AXIS_ITEMS:
        axis_item = plotItem.getAxis(axis)
        assert not axis_item.isVisible()
    assert not all(plotItem.vb.mouseEnabled())

    assert controller._plot.imageItem.image.shape == (30, 40)
    # Erase widget, a default with size 10, 10 is set
    controller.clear_widget()
    assert controller._plot.imageItem.image.shape == (10, 10)

    # teardown
    controller.destroy()
    assert controller.widget is None
