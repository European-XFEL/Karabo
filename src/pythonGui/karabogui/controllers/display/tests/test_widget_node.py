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
