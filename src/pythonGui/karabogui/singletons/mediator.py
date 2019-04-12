#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 24, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import defaultdict

from PyQt4.QtCore import QObject

from karabogui.events import KaraboBroadcastEvent


class Mediator(QObject):
    """The main dispatch point for KaraboBroadcastEvent events.

    See ``karabogui.events`` for more details.
    """

    def __init__(self, parent=None):
        super(Mediator, self).__init__(parent)

        self._listeners = defaultdict(set)

    def event(self, event):
        """Dispatch KaraboBroadcastEvents to all interested listeners.
        """
        if not isinstance(event, KaraboBroadcastEvent):
            return super(Mediator, self).event(event)

        # Make a copy to avoid a race condition in the case that processing
        # an event causes new listeners to be added
        sender = event.sender
        data = event.data
        handlers = list(self._listeners.get(sender, ()))
        for handler in handlers:
            handler(data)

        event.accept()
        return True

    def register_listener(self, event_map):
        """Add an event listener.
        """
        for event, handler in event_map.items():
            self._listeners[event].add(handler)

    def unregister_listener(self, event_map):
        """Remove an event listener.
        """
        for event, handler in event_map.items():
            self._listeners[event].remove(handler)
            if not self._listeners[event]:
                del self._listeners[event]
