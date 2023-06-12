#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 24, 2016
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
#############################################################################
from collections import defaultdict

from qtpy.QtCore import QObject

from karabogui.events import KaraboBroadcastEvent


class Mediator(QObject):
    """The main dispatch point for KaraboBroadcastEvent events.

    See ``karabogui.events`` for more details.
    """

    def __init__(self, parent=None):
        super().__init__(parent)

        self._listeners = defaultdict(set)

    def event(self, event):
        """Dispatch KaraboBroadcastEvents to all interested listeners.
        """
        if not isinstance(event, KaraboBroadcastEvent):
            return super().event(event)

        sender = event.sender
        data = event.data
        # Make a copy in case listeners are added or removed!
        handlers = list(self._listeners.get(sender, ()))
        for handler in handlers:
            handler(data)

        event.accept()
        return True

    def register_listener(self, event_map):
        """Add an event map.
        """
        for event, handler in event_map.items():
            self._listeners[event].add(handler)

    def unregister_listener(self, event_map):
        """Remove an event map.
        """
        for event, handler in event_map.items():
            # Note: We should not ask to remove a handler that is there ...
            # However, in mocking tests and not graceful stops of the GUI we
            # protect here!
            self._listeners[event].discard(handler)
            if not self._listeners[event]:
                del self._listeners[event]
