from PyQt4.QtGui import QLabel
from traits.api import Str, on_trait_change

from karabo.common.api import DeviceStatus
from karabo.middlelayer import Bool, Configurable, String, AccessMode
from karabo_gui.binding.api import (
    BaseBindingController, DeviceClassProxy, PropertyProxy, StringBinding,
    build_binding, register_binding_controller,
    KARABO_SCHEMA_DISPLAYED_NAME
)
from karabo_gui.testing import GuiTestCase


class SampleObject(Configurable):
    first = String(accessMode=AccessMode.READONLY, displayedName='First:')
    second = String(accessMode=AccessMode.READONLY)
    unsupported = Bool(accessMode=AccessMode.READONLY)


class ChangeObject(Configurable):
    first = String(accessMode=AccessMode.READONLY, displayedName='Change:')


@register_binding_controller(binding_type=StringBinding)
class SingleBindingController(BaseBindingController):
    disp_name = Str

    def create_widget(self, parent):
        return QLabel(parent)

    @on_trait_change('proxy.binding')
    def _binding_update(self, binding):
        self.disp_name = binding.attributes.get(KARABO_SCHEMA_DISPLAYED_NAME)

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        self.widget.setText(self.disp_name + value)


@register_binding_controller(binding_type=StringBinding)
class MultiBindingController(BaseBindingController):
    def add_proxy(self, proxy):
        pass  # Only need to avoid `raise NotImplementedError` here

    def create_widget(self, parent):
        return QLabel(parent)

    @on_trait_change('proxies:value')
    def _value_update(self, value):
        self.widget.setText(value)


class TestSingleBindingController(GuiTestCase):
    def setUp(self):
        super(TestSingleBindingController, self).setUp()

        schema = SampleObject.getClassSchema()
        binding = build_binding(schema)
        device = DeviceClassProxy(binding=binding, server_id='Fake',
                                  status=DeviceStatus.OFFLINE)
        self.first = PropertyProxy(root_proxy=device, path='first')
        self.second = PropertyProxy(root_proxy=device, path='second')
        self.unsupported = PropertyProxy(root_proxy=device, path='unsupported')

        # Create the controllers and initialize their widgets
        self.single = SingleBindingController(proxy=self.first)
        self.single.create(None)
        self.multi = MultiBindingController(proxy=self.first)
        assert self.multi.visualize_additional_property(self.second)
        self.multi.create(None)

    def tearDown(self):
        self.single.destroy()
        self.multi.destroy()
        assert self.single.widget is None
        assert self.multi.widget is None

    def test_extra_properties(self):
        # SingleBindingController doesn't support multiple properties
        assert not self.single.visualize_additional_property(self.second)

        # Use a temporary controller which won't affect other tests
        controller = MultiBindingController(proxy=self.first)
        # Adding an already watched property doesn't work
        assert not controller.visualize_additional_property(self.first)
        assert controller.visualize_additional_property(self.second)
        assert not controller.visualize_additional_property(self.second)
        assert not controller.visualize_additional_property(self.unsupported)

    def test_single_value_update(self):
        self.first.value = 'Foo'
        assert self.single.widget.text() == 'First:Foo'

    def test_multi_value_update(self):
        self.first.value = 'Bar'
        assert self.multi.widget.text() == 'Bar'
        self.second.value = 'Qux'
        assert self.multi.widget.text() == 'Qux'

    def test_schema_injection(self):
        self.first.value = 'Now'
        assert self.single.widget.text() == 'First:Now'

        try:
            # New device schema should cause a call to
            # SingleBindingController._binding_update
            schema = ChangeObject.getClassSchema()
            build_binding(schema, existing=self.first.root_proxy.binding)
            # Schema injection resets the value!
            self.first.value = 'Now'
            assert self.single.widget.text() == 'Change:Now'

        finally:
            # Put things back as they were!
            schema = SampleObject.getClassSchema()
            build_binding(schema, existing=self.first.root_proxy.binding)
