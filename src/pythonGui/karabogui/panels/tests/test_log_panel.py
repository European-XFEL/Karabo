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
from unittest import main

from karabo.native import Hash
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.panels.loggingpanel import LoggingPanel
from karabogui.singletons.mediator import Mediator
from karabogui.testing import GuiTestCase, singletons


class TestLogPanel(GuiTestCase):

    def test_log_panel(self):
        """Test the logging panel"""

        data = [Hash(
            "timestamp", "2021-07-05T15:08:35.250",
            "type", "INFO",
            "category", "Karabo_GuiServer_1",
            "message", "Hello!")]
        info = {'messages': data}

        mediator = Mediator()
        with singletons(mediator=mediator):
            panel = LoggingPanel()
            broadcast_event(KaraboEvent.LogMessages, info)
            self.process_qt_events()
            assert panel._log_widget.table_model.rowCount(None) == 1
            info = {'status': False}
            broadcast_event(KaraboEvent.NetworkConnectStatus, info)
            self.process_qt_events()
            assert panel._log_widget.table_model.rowCount(None) == 0


if __name__ == '__main__':
    main()
