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
from unittest import mock

from qtpy.QtWidgets import QLabel
from traits.api import Dict, Instance, Int, Str, Undefined

from karabo.common.api import KARABO_SCHEMA_DISPLAYED_NAME, State
from karabo.common.scenemodel.api import BaseWidgetObjectData
from karabo.native import AccessMode, Bool, Configurable, Hash, Node, String
from karabogui.binding.api import (
    DeviceClassProxy, NodeBinding, PropertyProxy, ProxyStatus, StringBinding,
    apply_configuration, build_binding, get_binding_value)
from karabogui.testing import (
    GuiTestCase, flushed_registry, get_property_proxy, set_proxy_value)

from ..base import BaseBindingController
from ..registry import register_binding_controller
from ..util import with_display_type


class SampleObject(Configurable):
    first = String(accessMode=AccessMode.READONLY, displayedName='First:')
    second = String(accessMode=AccessMode.READONLY)
    unsupported = Bool(accessMode=AccessMode.READONLY)


class ChangeObject(Configurable):
    first = String(accessMode=AccessMode.READONLY, displayedName='Change:')


class NodedObject(Configurable):
    node = Node(SampleObject)


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
        model = Instance(UniqueWidgetModel, args=())
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

    @register_binding_controller(klassname='Multi', binding_type=StringBinding,
                                 can_show_nothing=True)
    class MultiBindingController(BaseBindingController):
        model = Instance(UniqueWidgetModel, args=())

        def add_proxy(self, proxy):
            return True

        def remove_proxy(self, proxy):
            return True

        def binding_update(self, proxy):
            return True

        def create_widget(self, parent):
            return QLabel(parent)

        def value_update(self, proxy):
            value = get_binding_value(proxy)
            if value is not None:
                self.widget.setText(value)

    @register_binding_controller(klassname="Don't show nothing",
                                 binding_type=StringBinding,
                                 can_show_nothing=False)
    class DontShowNothingBindingController(BaseBindingController):
        model = Instance(UniqueWidgetModel, args=())

        def add_proxy(self, proxy):
            return True

        def binding_update(self, proxy):
            return True

        def create_widget(self, parent):
            return QLabel(parent)

        def value_update(self, proxy):
            self.widget.setText(proxy.value)

    @register_binding_controller(klassname='Node', binding_type=NodeBinding)
    class NodeBindingController(BaseBindingController):
        model = Instance(UniqueWidgetModel, args=())

        def create_widget(self, parent):
            return QLabel(parent)

        def binding_update(self, proxy):
            return True

        def value_update(self, proxy):
            self.widget.setText(proxy.binding.value.second.value)

    @register_binding_controller(klassname='Mono', binding_type=StringBinding)
    class SingleBindingController(BaseBindingController):
        model = Instance(UniqueWidgetModel, args=())
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
            value = get_binding_value(proxy)
            if value is not None:
                self.widget.setText(self.disp_name + proxy.value)

    @register_binding_controller(klassname='State', binding_type=StringBinding,
                                 is_compatible=with_display_type('State'))
    class StateTrackingController(BaseBindingController):
        model = Instance(UniqueWidgetModel, args=())

        def create_widget(self, parent):
            return QLabel(parent)

        def state_update(self, proxy):
            self.widget.setText(proxy.root_proxy.state_binding.value)

    return {
        'DeviceController': DeviceController,
        'MultiBindingController': MultiBindingController,
        'DontShowNothingBindingController': DontShowNothingBindingController,
        'NodeBindingController': NodeBindingController,
        'SingleBindingController': SingleBindingController,
        'StateTrackingController': StateTrackingController,
    }


class TestBaseBindingController(GuiTestCase):
    def setUp(self):
        super().setUp()

        # Avoid polluting the controller registry!
        with flushed_registry():
            klasses = _define_binding_classes()

        # Save the classes so they only need to be declared once
        self.DeviceController = klasses['DeviceController']
        self.MultiBindingController = klasses['MultiBindingController']
        self.NodeBindingController = klasses['NodeBindingController']
        self.SingleBindingController = klasses['SingleBindingController']
        self.StateTrackingController = klasses['StateTrackingController']
        self.DontShowNothingBindingController = \
            klasses['DontShowNothingBindingController']

        schema = SampleObject.getClassSchema()
        binding = build_binding(schema)
        device = DeviceClassProxy(binding=binding, server_id='Fake',
                                  status=ProxyStatus.OFFLINE)
        self.first = PropertyProxy(root_proxy=device, path='first')
        self.second = PropertyProxy(root_proxy=device, path='second')
        self.unsupported = PropertyProxy(root_proxy=device, path='unsupported')

        # Create the controllers and initialize their widgets
        self.single = self.SingleBindingController(proxy=self.first)
        self.single.create(None)
        self.single.finish_initialization()

        # The order of initializing a controller with multiple properties are
        # as follows:
        # 1. instantiate controller class
        # 2. controller.create(parent) with parent of QWidget type or None
        # 3. controller.visualize_additional_property(proxy) for addtl proxies
        # 4. controller.finish_initialization()
        self.multi = self.MultiBindingController(proxy=self.first)
        self.multi.create(None)
        assert self.multi.visualize_additional_property(self.second)
        self.multi.finish_initialization()

        # Create a mock  slot
        self.received_values = []

    def tearDown(self):
        self.single.destroy()
        self.multi.destroy()
        assert self.single.widget is None
        assert self.multi.widget is None

    def test_extra_properties(self):
        # SingleBindingController doesn't support multiple properties
        assert not self.single.visualize_additional_property(self.second)

        # Use a temporary controller which won't affect other tests
        model = UniqueWidgetModel(keys=['first'])
        with mock.patch.object(self.MultiBindingController, "binding_update") \
                as binding_update:
            controller = self.MultiBindingController(proxy=self.first,
                                                     model=model)
            controller.create(None)
            # Adding an already watched property doesn't work
            assert not controller.visualize_additional_property(self.first)
            assert controller.model.keys == ['first']

            binding_update.assert_called_once()
            binding_update.reset_mock()

            assert controller.visualize_additional_property(self.second)
            assert controller.model.keys == ['first', 'second']

            binding_update.assert_not_called()

            assert not controller.visualize_additional_property(
                self.second)
            assert not controller.visualize_additional_property(
                self.unsupported)
            assert controller.model.keys == ['first', 'second']
            binding_update.assert_not_called()

            # Remove an additional proxy
            assert self.second in controller.proxies
            controller.remove_additional_property(self.second)
            binding_update.assert_not_called()

            assert controller.model.keys == ['first']
            assert self.second not in controller.proxies

            # Already removed!
            assert not controller.remove_additional_property(self.second)

            # Value update is not effective anymore
            set_proxy_value(self.second, self.second.path, "NoUpdateShould")
            assert controller.widget.text() != "NoUpdateShould"

    def test_single_value_update(self):
        set_proxy_value(self.first, 'first', 'Foo')
        assert self.single.widget.text() == 'First:Foo'

    def test_multi_value_update(self):
        set_proxy_value(self.first, 'first', 'Bar')
        assert self.multi.widget.text() == 'Bar'
        set_proxy_value(self.second, 'second', 'Qux')
        assert self.multi.widget.text() == 'Qux'

    def test_node_value_update(self):
        binding = build_binding(NodedObject.getClassSchema())
        device = DeviceClassProxy(binding=binding, server_id='Test',
                                  status=ProxyStatus.OFFLINE)
        proxy = PropertyProxy(root_proxy=device, path='node')
        controller = self.NodeBindingController(proxy=proxy)
        controller.create(None)

        config = Hash('node', Hash('first', 'foo', 'second', 'bar'))
        apply_configuration(config, binding)
        assert controller.widget.text() == 'bar'

    def test_mocked_single_value_update(self):
        with mock.patch.object(self.SingleBindingController, "value_update",
                               side_effect=self._mocked_value_update) \
                as mocked_method:
            self._assert_value_update(proxy=self.first,
                                      value="Foo",
                                      value_update=mocked_method)

    def test_mocked_multi_value_update(self):
        with mock.patch.object(self.MultiBindingController, "value_update",
                               side_effect=self._mocked_value_update) \
                as mocked_method:
            self._assert_value_update(proxy=self.first,
                                      value="Bar",
                                      value_update=mocked_method)

            self._assert_value_update(proxy=self.second,
                                      value="Qux",
                                      value_update=mocked_method)

    def test_mocked_node_value_update(self):
        # Create a mocked value_update
        first_value, second_value = None, None

        def mocked_value_update(proxy):
            nonlocal first_value, second_value
            first_value = proxy.value.first.value
            second_value = proxy.value.second.value

        # Instantiate the node proxy and controller
        binding = build_binding(NodedObject.getClassSchema())
        device = DeviceClassProxy(binding=binding, server_id='Test',
                                  status=ProxyStatus.OFFLINE)
        proxy = PropertyProxy(root_proxy=device, path='node')
        controller = self.NodeBindingController(proxy=proxy)
        controller.create(None)

        # Trigger the value update
        with mock.patch.object(self.NodeBindingController, "value_update",
                               side_effect=mocked_value_update) \
                as value_update:
            config = Hash('node', Hash('first', 'foo', 'second', 'bar'))
            apply_configuration(config, binding)
            value_update.assert_called_once_with(proxy)
            assert first_value == 'foo'
            assert second_value == 'bar'

    def _mocked_value_update(self, proxy):
        self.received_values.append(proxy.value)

    def _assert_value_update(self, proxy, value, value_update):
        # Prepare the test properties
        value_update.reset_mock()
        self.received_values.clear()

        # Trigger value_update
        set_proxy_value(proxy, proxy.path, value)
        value_update.assert_called_once_with(proxy)
        assert self.received_values == [value]

    def test_init(self):
        # Check controller that can_show_nothing and without values
        self._assert_init_value_update(self.MultiBindingController,
                                       values=[Undefined, Undefined])

        # Check controller with can_show_nothing is False and without values
        self._assert_init_value_update(self.DontShowNothingBindingController,
                                       called=False)

        # Check controller with can_show_nothing is False and with proxies
        # having previous values
        first_value, second_value = 'Foo', 'Bar'
        set_proxy_value(self.first, self.first.path, first_value)
        set_proxy_value(self.second, self.second.path, second_value)
        self._assert_init_value_update(self.DontShowNothingBindingController,
                                       values=[second_value, first_value])

    def _assert_init_value_update(self, klass, called=True, values=None):
        """The initialization of the controller in the scene are done
        as follows:

        1. controller_klass(proxy, model) with the first proxy and model
        2. controller.create(parent) with parent as the container widget
        3. controller.visualize_additional_property(proxy) for every
           additional proxies
        4. controller.finish_initialization()
        """
        # Prepare the test properties
        self.received_values = []

        # Initialize the controller
        with mock.patch.object(klass, "value_update",
                               side_effect=self._mocked_value_update) \
                as value_update:
            controller = klass(proxy=self.first)
            controller.create(None)
            controller.visualize_additional_property(proxy=self.second)
            controller.finish_initialization()

        if called is False:
            value_update.assert_not_called()
        else:
            # We expect that the value_update is triggered by
            # additional proxies first, then the main proxy
            proxies = [self.second, self.first]
            calls = [mock.call(proxy) for proxy in proxies]
            value_update.assert_has_calls(calls)
            values = [proxy.value for proxy in proxies]
            assert self.received_values == values

    def test_deferred_update(self):
        deferred_before = self.single.deferred

        self.single.update_later()
        # Force the event loop to run so that the callback gets called
        self.process_qt_events()

        assert deferred_before + 1 == self.single.deferred

    def test_schema_injection(self):
        set_proxy_value(self.first, 'first', 'Now')
        assert self.single.widget.text() == 'First:Now'

        try:
            # New device schema should cause a call to
            # SingleBindingController._binding_update
            schema = ChangeObject.getClassSchema()
            build_binding(schema, existing=self.first.root_proxy.binding)
            # Schema injection resets the value!
            set_proxy_value(self.first, 'first', 'Now')
            assert self.single.widget.text() == 'Change:Now'

        finally:
            # Put things back as they were!
            schema = SampleObject.getClassSchema()
            build_binding(schema, existing=self.first.root_proxy.binding)

    def test_device_wakeup(self):
        # Build a widget with two proxies that have no `binding`
        binding = build_binding(Unconnected.getClassSchema())
        device = DeviceClassProxy(binding=binding, server_id='Test',
                                  status=ProxyStatus.OFFLINE)
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
                                  status=ProxyStatus.OFFLINE)
        # Make a proxy for something other than the 'state' property
        proxy = PropertyProxy(root_proxy=device, path='other')
        controller = self.StateTrackingController(proxy=proxy)
        controller.create(None)

        set_proxy_value(proxy, 'state', 'ERROR')
        assert controller.widget.text() == 'ERROR'

    def test_device_state_bad_binding(self):
        binding = build_binding(StateObject.getClassSchema())
        device = DeviceClassProxy(binding=binding, server_id='Test',
                                  status=ProxyStatus.OFFLINE)
        # Make a proxy for something which doesn't exist
        proxy = PropertyProxy(root_proxy=device, path='oaoijasdoijasd')
        controller = self.StateTrackingController(proxy=proxy)
        controller.create(None)

        set_proxy_value(proxy, 'state', 'ERROR')
        assert controller.widget.text() == ''

    def test_get_instance_id(self):
        proxy = get_property_proxy(None, "prop")
        controller = self.StateTrackingController(proxy=proxy)
        assert controller.getInstanceId() == "TestDevice"
