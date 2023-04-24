# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabogui.binding.builder import build_binding
from karabogui.binding.config import apply_configuration
from karabogui.binding.proxy import DeviceProxy, PropertyProxy
from karabogui.controllers.display.tests.image import (
    get_image_hash, get_pipeline_schema)
from karabogui.graph.common.const import AXIS_ITEMS
from karabogui.testing import GuiTestCase

from ..webcam_graph import DisplayWebCamGraph


class TestCase(GuiTestCase):
    def setUp(self):
        super(TestCase, self).setUp()

        schema = get_pipeline_schema()
        binding = build_binding(schema)
        root_proxy = DeviceProxy(binding=binding)

        self.output_proxy = PropertyProxy(root_proxy=root_proxy,
                                          path='output.data')
        self.img_proxy = PropertyProxy(root_proxy=root_proxy,
                                       path='output.data.image')
        self.controller = DisplayWebCamGraph(proxy=self.img_proxy)

    def tearDown(self):
        super(TestCase, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_basics(self):
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)

        image_hash = get_image_hash()
        apply_configuration(image_hash, self.output_proxy.binding)

        plotItem = self.controller._plot
        self.assertFalse(plotItem.menuEnabled())

        image_shape = list(plotItem.imageItem.image.shape)
        self.assertEqual(image_shape, image_hash['image']['dims'])

        # Assert axis are disabled
        for axis in AXIS_ITEMS:
            axis_item = plotItem.getAxis(axis)
            self.assertFalse(axis_item.isVisible())
        self.assertFalse(all(plotItem.vb.mouseEnabled()))
