# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.native import Configurable, Int32, Node
from karabogui.binding.api import PropertyProxy
from karabogui.testing import GuiTestCase, get_class_property_proxy

from ..widgetnode import DisplayWidgetNode


class DataNode(Configurable):
    displayType = "WidgetNode|DataNode"
    value = Int32(defaultValue=32)


class Object(Configurable):
    node = Node(DataNode)


class TestWidgetNode(GuiTestCase):
    def setUp(self):
        super(TestWidgetNode, self).setUp()

        schema = Object.getClassSchema()
        self.widget_node = get_class_property_proxy(schema, 'node')
        device = self.widget_node.root_proxy
        self.value = PropertyProxy(root_proxy=device, path='value')

        self.controller = DisplayWidgetNode(proxy=self.widget_node)
        self.controller.create(None)

    def tearDown(self):
        super(TestWidgetNode, self).tearDown()
        self.controller.destroy()
        assert self.controller.widget is None

    def test_widget_version(self):
        self.assertEqual(self.controller.widget.text(),
                         "WidgetNode\nNodeType: DataNode")
