import json

from karabo.common.api import KARABO_SCHEMA_DAQ_POLICY
from karabo.native import (
    AccessMode, Assignment, Configurable, Int8, Node, String, Unit)
from karabogui import icons
from karabogui.testing import GuiTestCase, get_class_property_proxy
from ..utils import (
    dragged_configurator_items, get_child_names, get_icon,
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


class TestConfiguratorUtils(GuiTestCase):
    def test_dragged_configurator_items(self):
        schema = Object.getClassSchema()
        proxy = get_class_property_proxy(schema, 'string')
        mime_data = dragged_configurator_items([proxy])
        items_data = mime_data.data('tree_items').data()
        drop = json.loads(items_data.decode())

        expected = {
            'label': 'String',
            'key': 'string',
            'display_widget_class': 'DisplayLabel',
            'edit_widget_class': 'EditableLineEdit'
        }
        assert drop == [expected]

    def test_get_child_names(self):
        schema = Object.getClassSchema()
        proxy = get_class_property_proxy(schema, 'integer')

        property_names = get_child_names(proxy)
        assert property_names == (KARABO_SCHEMA_DAQ_POLICY,)

        device_names = get_child_names(proxy.root_proxy)
        self.assertEqual(device_names,
                         ['string', 'integer', 'nodeMandatory',
                          'nodeNormal', 'nodeOfNode'])

    def test_get_mandatory(self):
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

    def test_get_icon(self):
        schema = Object.getClassSchema()
        proxy = get_class_property_proxy(schema, 'integer')
        assert get_icon(proxy.binding) == icons.int

        schema = Object.getClassSchema()
        proxy = get_class_property_proxy(schema, 'string')
        assert get_icon(proxy.binding) == icons.string

    def test_threshold_triggered(self):
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
