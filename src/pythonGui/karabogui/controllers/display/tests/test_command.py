from unittest.mock import Mock

from PyQt4.QtGui import QToolButton

from karabo.middlelayer import Configurable, Hash, State, Slot, String
from karabogui.binding.api import (
    DeviceProxy, PropertyProxy, build_binding, apply_configuration,
    apply_default_configuration
)
from karabogui.testing import GuiTestCase, singletons
from ..command import DisplayCommand


class EmptyDevice(Configurable):
    """What a device looks like to a scene when it's offline"""


class SlottedDevice(Configurable):
    state = String(defaultValue=State.INIT)

    @Slot(allowedStates=[State.INIT],
          displayedName='Call ME')
    def callme(self):
        pass

    @Slot(allowedStates=[State.ACTIVE])
    def yep(self):
        pass


def get_slot_proxy(additional=False, klass=SlottedDevice):
    schema = klass.getClassSchema()
    binding = build_binding(schema)
    apply_default_configuration(binding)

    device = DeviceProxy(device_id='dev', server_id='Test', binding=binding)
    slot_proxy = PropertyProxy(root_proxy=device, path='callme')
    if additional:
        return slot_proxy, PropertyProxy(root_proxy=device, path='yep')
    else:
        return slot_proxy


class TestDisplayCommand(GuiTestCase):
    def test_basics(self):
        slot_proxy = get_slot_proxy()
        controller = DisplayCommand(proxy=slot_proxy)
        controller.create(None)

        assert controller.widget is not None
        assert isinstance(controller._button, QToolButton)
        assert controller._actions[0].action.text() == 'Call ME'

        controller.destroy()
        assert controller.widget is None

    def test_trigger(self):
        slot_proxy = get_slot_proxy()
        controller = DisplayCommand(proxy=slot_proxy)
        controller.create(None)
        network = Mock()
        with singletons(network=network):
            controller._actions[0].action.trigger()
            network.onExecute.assert_called_with('dev', 'callme')

    def test_additional_proxy(self):
        slot_p1, slot_p2 = get_slot_proxy(additional=True)
        controller = DisplayCommand(proxy=slot_p1)
        controller.create(None)

        assert controller.visualize_additional_property(slot_p2)
        # no displayedName, should be path
        assert controller._actions[1].action.text() == 'yep'

    def test_state_change(self):
        schema = SlottedDevice.getClassSchema()
        binding = build_binding(schema)
        apply_default_configuration(binding)

        dev_proxy = DeviceProxy(device_id='dev', server_id='swerver',
                                binding=binding)
        slot_proxy = PropertyProxy(root_proxy=dev_proxy, path='yep')
        controller = DisplayCommand(proxy=slot_proxy)
        controller.create(None)

        # device state is INIT
        assert not controller._actions[0].action.isEnabled()

        config = Hash("state", State.ACTIVE.value)
        apply_configuration(config, binding)
        # now `yep` should be enabled
        assert controller._actions[0].action.isEnabled()

    def test_button_finalization(self):
        slot1, slot2 = get_slot_proxy(additional=True, klass=EmptyDevice)

        controller = DisplayCommand(proxy=slot1)
        controller.create(None)
        controller.visualize_additional_property(slot2)

        assert controller._actions[0].action.text() == 'NO TEXT'
        assert controller._actions[1].action.text() == 'NO TEXT'

        # The device "comes online"
        schema = SlottedDevice.getClassSchema()
        build_binding(schema, existing=slot1.root_proxy.binding)

        assert controller._actions[0].action.text() == 'Call ME'
        assert controller._actions[1].action.text() == 'yep'
