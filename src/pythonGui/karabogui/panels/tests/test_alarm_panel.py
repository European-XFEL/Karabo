from unittest import mock

from karabogui.alarms.api import INIT_UPDATE_TYPE, AlarmModel
from karabogui.alarms.tests.test_alarm_model import create_alarm_data
from karabogui.events import KaraboEvent
from karabogui.panels.alarmpanel import AlarmPanel
from karabogui.testing import GuiTestCase, singletons


class TestAlarmPanel(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.model = AlarmModel()
        with singletons(alarm_model=self.model):
            self.panel = AlarmPanel()
            self.model.init_alarms_info(create_alarm_data(INIT_UPDATE_TYPE))
        container = mock.Mock()
        container.maximized.new = False
        self.panel.attach_to_container(container)
        self.panel.show()

    def tearDown(self):
        super().tearDown()
        self.panel.close()
        self.panel.destroy()
        self.model = None
        self.panel = None

    def test_panel_delegate(self):
        # The acknowledge column is 7
        index = self.model.createIndex(0, 7)
        self.assertTrue(self.panel.delegate.isEnabled(index))
        # Last row has no ack in data, but -1 for 0 start
        rows = self.model.rowCount()
        index = self.model.createIndex(rows - 1, 7)
        self.assertFalse(self.panel.delegate.isEnabled(index))

        # Second last row cannot be acked as well, allowed to ack, but no
        # available at the moment
        rows = self.model.rowCount()
        index = self.model.createIndex(rows - 2, 7)
        self.assertFalse(self.panel.delegate.isEnabled(index))

        network = mock.Mock()
        with singletons(network=network):
            index = self.model.createIndex(0, 7)
            self.panel.delegate.click_action(index)
            network.onAcknowledgeAlarm.assert_called_once()

    def test_panel_click(self):
        path = 'karabogui.panels.alarmpanel.broadcast_event'
        with mock.patch(path) as broadcast:
            index = self.model.createIndex(0, 7)
            self.panel.onRowDoubleClicked(index)
            broadcast.assert_called_with(KaraboEvent.ShowDevice,
                                         {'deviceId': 'dev0'})

    def test_filter_button_toggle(self):
        self.assertEqual(self.model.rowCount(), 13)

        filterModel = self.panel.model
        alarm_button = self.panel.ui_show_alarm_warn
        self.panel.filterToggled(alarm_button)
        self.assertEqual(filterModel.rowCount(), 10)

        interlock_button = self.panel.ui_show_interlock
        self.panel.filterToggled(interlock_button)
        self.assertEqual(filterModel.rowCount(), 3)