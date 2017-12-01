from PyQt4.QtGui import QLabel
from traits.api import Dict, Instance, Int, Str

from karabo.common.api import DeviceStatus, State
from karabo.common.scenemodel.api import BaseWidgetObjectData
from karabo.middlelayer import Bool, Configurable, String, AccessMode
from karabogui.binding.api import (
    DeviceClassProxy, PropertyProxy, StringBinding, build_binding,
    KARABO_SCHEMA_DISPLAYED_NAME
)
from karabogui.testing import GuiTestCase, flushed_registry
from ..base import BaseBindingController
from ..registry import register_binding_controller
from ..util import with_display_type


class SampleObject(Configurable):
    first = String(accessMode=AccessMode.READONLY, displayedName='First:')
    second = String(accessMode=AccessMode.READONLY)
    unsupported = Bool(accessMode=AccessMode.READONLY)


class ChangeObject(Configurable):
    first = String(accessMode=AccessMode.READONLY, displayedName='Change:')


class StateObject(Configurable):
    state = String(enum=State, displayType='State')
    other = String()


# Classes for testing late binding initialization
class Unconnected(Configurable):
    pass


class Connected(Configurable):
    one = String(displayedName='One:')
    two = String(displayedName='Two:')


class UniqueWidgetModel(BaseWidgetObjectData):
    pass  # Satisfy the uniqueness check in register_binding_controller


def _define_binding_classes():
    """Do this in a function so that we can avoid polluting the registry
    """
    @register_binding_controller(klassname='Norm', binding_type=StringBinding)
    class DeviceController(BaseBindingController):
        model = Instance(UniqueWidgetModel)
        display_names = Dict

        def add_proxy(self, proxy):
            self._set_display_name(proxy)
            return True

        def binding_update(self, proxy):
            self._set_display_name(proxy)

        def create_widget(self, parent):
            return QLabel(parent)

        def _set_display_name(self, proxy):
            binding = proxy.binding
            if binding is not None:
                name = binding.attributes.get(KARABO_SCHEMA_DISPLAYED_NAME, '')
            else:
                name = ''
            self.display_names[proxy] = name

    @register_binding_controller(klassname='Multi', binding_type=StringBinding)
    class MultiBindingController(BaseBindingController):
        model = Instance(UniqueWidgetModel)

        def add_proxy(self, proxy):
            return True

        def create_widget(self, parent):
            return QLabel(parent)

        def value_update(self, proxy):
            self.widget.setText(proxy.value)

    @register_binding_controller(klassname='Mono', binding_type=StringBinding)
    class SingleBindingController(BaseBindingController):
        model = Instance(UniqueWidgetModel)
        disp_name = Str
        deferred = Int(0)

        def binding_update(self, proxy):
            attrs = proxy.binding.attributes
            self.disp_name = attrs.get(KARABO_SCHEMA_DISPLAYED_NAME)

        def create_widget(self, parent):
            return QLabel(parent)

        def deferred_update(self):
            self.deferred += 1

        def value_update(self, proxy):
            self.widget.setText(self.disp_name + proxy.value)

    @register_binding_controller(klassname='State', binding_type=StringBinding,
                                 is_compatible=with_display_type('State'))
    class StateTrackingController(BaseBindingController):
        model = Instance(UniqueWidgetModel)

        def create_widget(self, parent):
            return QLabel(parent)

        def state_update(self, proxy):
            self.widget.setText(proxy.root_proxy.state_binding.value)

    return {
        'DeviceController': DeviceController,
        'MultiBindingController': MultiBindingController,
        'SingleBindingController': SingleBindingController,
        'StateTrackingController': StateTrackingController,
    }


class TestBaseBindingController(GuiTestCase):
    def setUp(self):
        super(TestBaseBindingController, self).setUp()

        # Avoid polluting the controller registry!
        with flushed_registry():
            klasses = _define_binding_classes()

        # Save the classes so they only need to be declared once
        self.DeviceController = klasses['DeviceController']
        self.MultiBindingController = klasses['MultiBindingController']
        self.SingleBindingController = klasses['SingleBindingController']
        self.StateTrackingController = klasses['StateTrackingController']

        schema = SampleObject.getClassSchema()
        binding = build_binding(schema)
        device = DeviceClassProxy(binding=binding, server_id='Fake',
                                  status=DeviceStatus.OFFLINE)
        self.first = PropertyProxy(root_proxy=device, path='first')
        self.second = PropertyProxy(root_proxy=device, path='second')
        self.unsupported = PropertyProxy(root_proxy=device, path='unsupported')

        # Create the controllers and initialize their widgets
        self.single = self.SingleBindingController(proxy=self.first)
        self.single.create(None)
        self.multi = self.MultiBindingController(proxy=self.first)
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
        controller = self.MultiBindingController(proxy=self.first)
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

    def test_deferred_update(self):
        deferred_before = self.single.deferred

        self.single.update_later()
        # Force the event loop to run so that the callback gets called
        self.process_qt_events()

        assert deferred_before + 1 == self.single.deferred

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

    def test_device_wakeup(self):
        # Build a widget with two proxies that have no `binding`
        binding = build_binding(Unconnected.getClassSchema())
        device = DeviceClassProxy(binding=binding, server_id='Test',
                                  status=DeviceStatus.OFFLINE)
        one = PropertyProxy(root_proxy=device, path='one')
        two = PropertyProxy(root_proxy=device, path='two')
        controller = self.DeviceController(proxy=one)
        controller.create(None)

        controller.visualize_additional_property(two)
        assert two in controller.display_names

        # Then swap the schema so that the bindings are now available
        build_binding(Connected.getClassSchema(), existing=device.binding)
        assert controller.display_names[one] == 'One:'
        assert controller.display_names[two] == 'Two:'

    def test_device_state_tracking(self):
        binding = build_binding(StateObject.getClassSchema())
        device = DeviceClassProxy(binding=binding, server_id='Test',
                                  status=DeviceStatus.OFFLINE)
        # Make a proxy for something other than the 'state' property
        proxy = PropertyProxy(root_proxy=device, path='other')
        controller = self.StateTrackingController(proxy=proxy)
        controller.create(None)

        device.state_binding.value = 'ERROR'
        assert controller.widget.text() == 'ERROR'
