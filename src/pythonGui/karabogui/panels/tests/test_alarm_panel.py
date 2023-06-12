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

from karabogui.alarms.api import INIT_UPDATE_TYPE, AlarmModel
from karabogui.alarms.tests.test_alarm_model import create_alarm_data
from karabogui.events import KaraboEvent
from karabogui.panels.alarmpanel import AlarmPanel
from karabogui.testing import GuiTestCase, singletons

ACK_COLUMN = 6


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
        # The acknowledge column is 6
        index = self.model.createIndex(0, ACK_COLUMN)
        self.assertTrue(self.panel.delegate.isEnabled(index))
        # Last row has no ack in data, but -1 for 0 start
        rows = self.model.rowCount()
        index = self.model.createIndex(rows - 1, ACK_COLUMN)
        self.assertFalse(self.panel.delegate.isEnabled(index))

        # Second last row cannot be acked as well, allowed to ack, but no
        # available at the moment
        rows = self.model.rowCount()
        index = self.model.createIndex(rows - 2, ACK_COLUMN)
        self.assertFalse(self.panel.delegate.isEnabled(index))

        network = mock.Mock()
        with singletons(network=network):
            index = self.model.createIndex(0, ACK_COLUMN)
            self.panel.delegate.click_action(index)
            network.onAcknowledgeAlarm.assert_called_once()

    def test_panel_click(self):
        path = 'karabogui.panels.alarmpanel.broadcast_event'
        with mock.patch(path) as broadcast:
            index = self.model.createIndex(0, ACK_COLUMN)
            self.panel.onRowDoubleClicked(index)
            broadcast.assert_called_with(KaraboEvent.ShowDevice,
                                         {'deviceId': 'dev0'})

    def test_filter_button_toggle(self):
        self.assertEqual(self.model.rowCount(), 13)

        filterModel = self.panel.model
        self.panel.ui_show_alarm_warn.setChecked(True)
        self.panel.onFilterChanged()
        self.assertEqual(filterModel.rowCount(), 10)

        self.panel.ui_show_interlock.setChecked(True)
        self.panel.onFilterChanged()
        self.assertEqual(filterModel.rowCount(), 3)
