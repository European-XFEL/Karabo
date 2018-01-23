import json

from karabo.common.api import (
    KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, KARABO_SCHEMA_UNIT_SYMBOL,
    KARABO_SCHEMA_DAQ_POLICY)
from karabo.middlelayer import AccessMode, Configurable, Int8, String, Unit
from karabogui import icons
from karabogui.testing import GuiTestCase, get_class_property_proxy
from ..utils import (
    dragged_configurator_items, get_child_names, get_icon)


class Object(Configurable):
    string = String(displayedName='String',
                    accessMode=AccessMode.RECONFIGURABLE)
    integer = Int8(unitSymbol=Unit.METER)


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
        assert property_names == (KARABO_SCHEMA_METRIC_PREFIX_SYMBOL,
                                  KARABO_SCHEMA_UNIT_SYMBOL,
                                  KARABO_SCHEMA_DAQ_POLICY)

        device_names = get_child_names(proxy.root_proxy)
        assert device_names == ['string', 'integer']

    def test_get_icon(self):
        schema = Object.getClassSchema()
        proxy = get_class_property_proxy(schema, 'integer')
        assert get_icon(proxy.binding) == icons.int

        schema = Object.getClassSchema()
        proxy = get_class_property_proxy(schema, 'string')
        assert get_icon(proxy.binding) == icons.string
