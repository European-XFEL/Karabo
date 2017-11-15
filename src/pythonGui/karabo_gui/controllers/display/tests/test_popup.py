from unittest.mock import patch, Mock

from PyQt4.QtGui import QMessageBox

from karabo.common.api import State
from karabo.middlelayer import Configurable, String, Slot
from karabo_gui.binding.api import (
    DeviceProxy, PropertyProxy, apply_default_configuration, build_binding)
from karabo_gui.testing import GuiTestCase, singletons
from ..popup import PopUp


class MockMessageBox(QMessageBox):
    def setVisible(self, visible):
        self.visible = visible


class Object(Configurable):
    state = String(defaultValue=State.INIT)
    prop = String(displayedName='Tests')

    @Slot(allowedStates=[State.INIT])
    def ok(self):
        pass

    @Slot(allowedStates=[State.INIT])
    def cancel(self):
        pass

    @Slot()
    def extra(self):
        pass


class TestPopUp(GuiTestCase):
    def setUp(self):
        super(TestPopUp, self).setUp()

        schema = Object.getClassSchema()
        binding = build_binding(schema)
        apply_default_configuration(binding)
        device = DeviceProxy(device_id='dev', server_id='Test',
                             binding=binding)

        self.proxy = PropertyProxy(path='prop', root_proxy=device)
        self.ok_proxy = PropertyProxy(path='ok', root_proxy=device)
        self.cancel_proxy = PropertyProxy(path='cancel', root_proxy=device)
        self.extra_proxy = PropertyProxy(path='extra', root_proxy=device)

    def test_basics(self):
        controller = PopUp(proxy=self.proxy)
        controller.create(None)
        assert controller.widget is not None

        assert controller._dialog.windowTitle() == 'Tests'

        assert controller.visualize_additional_property(self.ok_proxy)
        assert controller.visualize_additional_property(self.cancel_proxy)
        assert not controller.visualize_additional_property(self.extra_proxy)

        controller.destroy()
        assert controller.widget is None

    def test_set_value(self):
        sym = 'karabo_gui.controllers.display.popup.QMessageBox'
        with patch(sym, new=MockMessageBox):
            controller = PopUp(proxy=self.proxy)
            controller.create(None)

            self.proxy.value = 'SOME TEXT'
            assert controller._dialog.visible

            self.proxy.value = ''
            assert not controller._dialog.visible

    def test_button_actions(self):
        controller = PopUp(proxy=self.proxy)
        controller.create(None)
        controller.visualize_additional_property(self.ok_proxy)

        network = Mock()
        with singletons(network=network):
            controller._on_finished(QMessageBox.Ok)
            network.onExecute.assert_called_with('dev', 'ok')

            network.reset_mock()
            controller._on_finished(QMessageBox.Cancel)
            assert not network.onExecute.called

            network.reset_mock()
            controller.visualize_additional_property(self.cancel_proxy)
            controller._on_finished(QMessageBox.Cancel)
            network.onExecute.assert_called_with('dev', 'cancel')
