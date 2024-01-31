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
from unittest import main, mock

from qtpy.QtCore import QDateTime, Qt

from karabogui.dialogs.api import ConfigurationFromPastDialog
from karabogui.events import KaraboEvent
from karabogui.singletons.mediator import Mediator
from karabogui.testing import GuiTestCase, singletons


class TestConfigurationFromPastDialog(GuiTestCase):

    def test_config_past_dialog(self):
        mediator = Mediator()
        network = mock.Mock()
        mediator.register_listener = mock.Mock()
        mediator.unregister_listener = mock.Mock()
        with singletons(mediator=mediator, network=network):
            dialog = ConfigurationFromPastDialog(instance_id="divvy")
            assert dialog.ui_instance_id.text() == "divvy"
            # Dialog registers to mediator
            mediator.register_listener.assert_called_once()

            # Clicking the show device requests navigation panel to show
            path = "karabogui.dialogs.configuration_from_past.broadcast_event"
            with mock.patch(path) as broadcast:
                self.click(dialog.ui_show_device)
                broadcast.assert_called_with(
                    KaraboEvent.ShowDevice, {"deviceId": "divvy",
                                             "showTopology": True})

            time = QDateTime.fromString("2021-07-15 09:07:42",
                                        "yyyy-MM-dd HH:mm:ss")
            dialog.ui_timepoint.setDateTime(time)
            self.click(dialog.ui_request)
            network.onGetConfigurationFromPast.assert_called_with(
                "divvy", preview=False, time=time.toUTC().toString(Qt.ISODate))

            # Click different time buttons
            for button in ("ui_one_week", "ui_one_day",
                           "ui_one_hour", "ui_ten_minutes"):
                prev_time = dialog.ui_timepoint.dateTime()
                self.click(getattr(dialog, button))
                self.process_qt_events()
                assert dialog.ui_timepoint.dateTime() != prev_time

            # Unregister and set successful
            dialog.done(1)
            mediator.unregister_listener.assert_called_once()

            # Accept dialog also requests
            dialog.ui_timepoint.setDateTime(time)
            dialog.accept()
            network.onGetConfigurationFromPast.assert_called_with(
                "divvy", preview=False, time=time.toUTC().toString(Qt.ISODate))

            # Network event and close
            dialog._event_network({"status": False})


if __name__ == "__main__":
    main()
