# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.native import Configurable, Int32, Node
from karabogui.testing import get_class_property_proxy

from ..widgetnode import DisplayWidgetNode


class DataNode(Configurable):
    displayType = "WidgetNode|DataNode"
    value = Int32(defaultValue=32)


class Object(Configurable):
    node = Node(DataNode)


def test_widgetnode_version(gui_app):
    # setup
    schema = Object.getClassSchema()
    widget_node = get_class_property_proxy(schema, 'node')
    controller = DisplayWidgetNode(proxy=widget_node)
    controller.create(None)

    # test body
    assert controller.widget.text() == "WidgetNode\nNodeType: DataNode"

    # teardown
    controller.destroy()
    assert controller.widget is None
