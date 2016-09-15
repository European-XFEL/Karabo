from enum import Enum

from PyQt4.QtCore import QEvent, QObject
from PyQt4.QtGui import QApplication

# Enum for karabo broadcast event senders
class KaraboEventSender(Enum):
    AlarmInitReply = "Alarm init reply"
    AlarmUpdate = "Alarm update"
    ConnectMacroInstance = "Connect macro instance"
    DeviceInitReply = "Device instantiate reply"
    OpenMacro = "Open macro"
    OpenSceneLink = "Scene link"
    OpenSceneView = "Open scene view"
    RemoveMacro = "Remove macro"
    RemoveSceneView = "Remove scene view"
    RenameSceneView = "Rename scene view"
    RenameMacro = "Rename macro"

# This is the global singleton for the karabo_mediator function.
_karabo_mediator = None


class KaraboBroadcastEvent(QEvent):
    """ Custom event to handle GUI widget communication """

    # Ask Qt for our event type
    Type = QEvent.Type(QEvent.registerEventType())

    def __init__(self, sender="", data=None):
        super(KaraboBroadcastEvent, self).__init__(self.Type)
        self.sender = sender  # Names the sender
        self.data = data or {}  # Includes the data which is sent


def __get_mediator():
    """ Return the karabo mediator singleton.
    """
    global _karabo_mediator
    if _karabo_mediator is None:
        _karabo_mediator = QObject()

    return _karabo_mediator


def broadcast_event(event):
    """ Broadcast the given `event`.
    """
    mediator = __get_mediator()
    QApplication.postEvent(mediator, event)


def register_for_broadcasts(qobject):
    """ Register the given `qobject` to the events coming from the singleton
        `mediator`.
    """
    mediator = __get_mediator()
    mediator.installEventFilter(qobject)


def unregister_from_broadcasts(qobject):
    """ Unregister the given `qobject` from the events coming from the
        singleton mediator object.
    """
    mediator = __get_mediator()
    mediator.removeEventFilter(qobject)
