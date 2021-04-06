from unittest.mock import Mock, patch

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QToolButton
from karabo.common.states import State
from karabo.native import AccessLevel, Configurable, Slot, String
from karabogui.binding.api import (
    DeviceProxy, PropertyProxy, build_binding, apply_default_configuration)
from karabogui.testing import GuiTestCase, set_proxy_value, singletons


class EmptyDevice(Configurable):
    """What a device looks like to a scene when it's offline"""


class SlottedDevice(Configurable):
    state = String(defaultValue=State.INIT)

    @Slot(allowedStates=[State.INIT],
          displayedName='Call ME',
          requiredAccessLevel=AccessLevel.OBSERVER)
    def callme(self):
        pass

    @Slot(allowedStates=[State.ACTIVE],
          requiredAccessLevel=AccessLevel.OBSERVER)
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


class TestDisplayIconCommand(GuiTestCase):

    def test_basics(self):
        """Test the basics of the display icon command"""
        from ..icon_command import DisplayIconCommand

        slot_proxy = get_slot_proxy()
        controller = DisplayIconCommand(proxy=slot_proxy)
        controller.create(None)

        assert controller.widget is not None
        assert isinstance(controller._button, QToolButton)

        controller.destroy()
        assert controller.widget is None

    def test_trigger(self):
        from ..icon_command import DisplayIconCommand

        slot_proxy = get_slot_proxy()
        controller = DisplayIconCommand(proxy=slot_proxy)
        controller.create(None)
        network = Mock()
        with singletons(network=network):
            controller._button.clicked.emit(True)
            network.onExecute.assert_called_with('dev', 'callme', False)

    def test_state_change(self):
        from ..icon_command import DisplayIconCommand

        schema = SlottedDevice.getClassSchema()
        binding = build_binding(schema)
        apply_default_configuration(binding)

        dev_proxy = DeviceProxy(device_id='dev', server_id='swerver',
                                binding=binding)
        slot_proxy = PropertyProxy(root_proxy=dev_proxy, path='yep')
        controller = DisplayIconCommand(proxy=slot_proxy)
        controller.create(None)

        # device state is INIT
        assert not controller._button.isEnabled()

        set_proxy_value(slot_proxy, 'state', State.ACTIVE.value)
        # now `yep` should be enabled
        assert controller._button.isEnabled()

    def test_icon_change(self):
        from ..icon_command import DisplayIconCommand, BUTTON_ICONS

        slot_proxy = get_slot_proxy()
        controller = DisplayIconCommand(proxy=slot_proxy)
        controller.create(None)

        actions = controller.widget.actions()
        assert len(actions) == 1

        exec_path = ("karabogui.controllers.display.icon_command."
                     "IconSelectionDialog.exec_")
        new_icon = 'Left'

        def exec_dialog(self):
            self.icon = BUTTON_ICONS[new_icon]
            self.name = new_icon

        mocked_exec = patch(exec_path, exec_dialog)
        with mocked_exec:
            actions[0].trigger()
            assert controller._icon == BUTTON_ICONS[new_icon]
            assert controller.model.icon_name == new_icon


class TestIconSelectionDialog(GuiTestCase):

    def setUp(self):
        from ..icon_command import IconSelectionDialog

        super(TestIconSelectionDialog, self).setUp()
        self.dialog = IconSelectionDialog()

    def tearDown(self):
        super(TestIconSelectionDialog, self).tearDown()
        self.dialog.destroy()

    def test_init(self):
        from ..icon_command import BUTTON_ICONS, NO_SELECTION

        assert self.dialog.icon == NO_SELECTION
        assert self.dialog.name == ''

        model = self.dialog.list_view.model()
        for row in range(model.rowCount()):
            index = model.index(row, 0)
            name = index.data(Qt.DisplayRole)

            assert name in BUTTON_ICONS

    def test_valid_double_click(self):
        from ..icon_command import BUTTON_ICONS
        new_icon = 'Left'

        def mocked_returns(role):
            if role == Qt.DisplayRole:
                return new_icon
            if role == Qt.UserRole + 1:
                return BUTTON_ICONS[new_icon]

        mocked_index = Mock()
        mocked_index.data.side_effect = mocked_returns
        self.dialog.handleDoubleClick(mocked_index)

        assert self.dialog.icon == BUTTON_ICONS[new_icon]
        assert self.dialog.name == new_icon

    def test_invalid_double_click(self):
        from ..icon_command import NO_SELECTION

        def mocked_returns(role):
            if role == Qt.DisplayRole:
                return None
            if role == Qt.UserRole + 1:
                return None

        mocked_index = Mock()
        mocked_index.data.side_effect = mocked_returns
        self.dialog.handleDoubleClick(mocked_index)

        assert self.dialog.icon == NO_SELECTION
        assert self.dialog.name == ''
