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
import json

from qtpy.QtGui import QColor

from karabo.native import (
    AccessLevel, AccessMode, Assignment, Configurable, Int8, Node, String,
    Unit)
from karabogui import icons
from karabogui.testing import get_class_property_proxy

from ..utils import (
    dragged_configurator_items, get_child_names, get_icon, get_qcolor_state,
    is_mandatory, threshold_triggered)


class MandatoryNode(Configurable):
    integer = Int8(
        unitSymbol=Unit.METER,
        assignment=Assignment.MANDATORY)


class NormalNode(Configurable):
    integer = Int8(
        unitSymbol=Unit.METER)


class NodeOfNode(Configurable):
    data = Node(MandatoryNode)


class Object(Configurable):
    string = String(
        displayedName='String',
        accessMode=AccessMode.RECONFIGURABLE)

    integer = Int8(
        unitSymbol=Unit.METER)

    nodeMandatory = Node(MandatoryNode)
    nodeNormal = Node(NormalNode)
    nodeOfNode = Node(NodeOfNode)


def test_dragged_configurator_items():
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, 'string')
    mime_data = dragged_configurator_items([proxy])
    items_data = mime_data.data('tree_items').data()
    drop = json.loads(items_data.decode())

    expected = {
        'label': 'String',
        'key': 'string',
        'type': 'Leaf',
        'display_widget_class': 'DisplayLabel',
        'edit_widget_class': 'EditableLineEdit'
    }
    assert drop == [expected]


def test_get_child_names():
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, 'integer')

    property_names = get_child_names(proxy, AccessLevel.EXPERT)
    # Attributes are empty for this proxy
    assert property_names == []

    device_names = get_child_names(proxy.root_proxy, AccessLevel.EXPERT)
    assert device_names == ['string', 'integer', 'nodeMandatory',
                            'nodeNormal', 'nodeOfNode']


def test_get_mandatory():
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, 'integer')
    assert is_mandatory(proxy.binding) is False

    proxy = get_class_property_proxy(schema, 'nodeNormal.integer')
    assert is_mandatory(proxy.binding) is False

    proxy = get_class_property_proxy(schema, 'nodeMandatory.integer')
    assert is_mandatory(proxy.binding) is True

    proxy = get_class_property_proxy(schema, 'nodeOfNode')
    assert is_mandatory(proxy.binding) is True

    proxy = get_class_property_proxy(schema, 'nodeMandatory')
    assert is_mandatory(proxy.binding) is True


def test_get_icon():
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, 'integer')
    assert get_icon(proxy.binding) == icons.int

    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, 'string')
    assert get_icon(proxy.binding) == icons.string


def test_threshold_triggered():
    # check the low limit
    value = 1
    high_limit = None
    low_limit = 2
    assert threshold_triggered(value, low_limit, high_limit) is True
    value = 3
    assert threshold_triggered(value, low_limit, high_limit) is False
    low_limit = None
    assert threshold_triggered(value, low_limit, high_limit) is False

    # check the high limit
    value = 3
    high_limit = 2
    low_limit = 1
    assert threshold_triggered(value, low_limit, high_limit) is True

    high_limit = 3
    assert threshold_triggered(value, low_limit, high_limit) is False
    low_limit = None
    assert threshold_triggered(value, low_limit, high_limit) is False


def test_stateq_color():
    color = get_qcolor_state("UNKNOWN")
    assert isinstance(color, QColor)
    assert color.getRgb() == (255, 200, 150, 128)

    color = get_qcolor_state("banana")
    assert color.getRgb() == (225, 242, 225, 128)

    color = get_qcolor_state("ERROR")
    assert color.getRgb() == (255, 155, 155, 128)
