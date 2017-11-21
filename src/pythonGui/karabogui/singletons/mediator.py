#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 24, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QObject

from karabogui.events import KaraboBroadcastEvent


class Mediator(QObject):
    """The main dispatch point for KaraboBroadcastEvent events.

    See ``karabogui.events`` for more details.
    """

    def __init__(self, parent=None):
        super(Mediator, self).__init__(parent)

        self._listeners = set()

    def event(self, event):
        """Dispatch KaraboBroadcastEvents to all interested listeners.
        """
        if not isinstance(event, KaraboBroadcastEvent):
            return super(Mediator, self).event(event)

        # Make a copy to avoid a race condition in the case that processing
        # an event causes new listeners to be added
        handlers = list(self._listeners)
        for obj in handlers:
            if obj.karaboBroadcastEvent(event):
                break

        event.accept()
        return True

    def register_listener(self, listener):
        """Add an event listener.
        """
        handler_method = getattr(listener, 'karaboBroadcastEvent', None)
        if handler_method is None or not callable(handler_method):
            msg = ('An object MUST implement a "karaboBroadcastEvent" method '
                   'if it registers for broadcasts!')
            raise RuntimeError(msg)

        self._listeners.add(listener)

    def unregister_listener(self, listener):
        """Remove an event listener.
        """
        self._listeners.remove(listener)
