from enum import Enum

from PyQt4.QtCore import QEvent
from PyQt4.QtGui import QApplication

from karabo_gui.singletons.api import get_mediator


# Enum for karabo broadcast event senders
class KaraboEventSender(Enum):
    AlarmInitReply = "Alarm init reply"
    AlarmUpdate = "Alarm update"
    AlarmDeviceUpdate = "Alarm device update"
    ConnectMacroInstance = "Connect macro instance"
    DeviceDataReceived = "Device class/schema/config received"
    DeviceInitReply = "Device instantiate reply"
    DeviceErrorChanged = "Device error state changed"
    DeviceStateChanged = "Device state changed"
    LogMessages = "Log messages"
    NetworkConnectStatus = "Network connection changed"
    NotificationMessage = "Notification message"
    OpenMacro = "Open macro"
    OpenSceneLink = "Scene link"
    OpenSceneView = "Open scene view"
    RemoveAlarmServices = "Remove alarm services"
    RemoveMacro = "Remove macro"
    RemoveSceneView = "Remove scene view"
    RenameSceneView = "Rename scene view"
    RenameMacro = "Rename macro"
    ProjectDomainsList = "Project domains list"
    ProjectItemsList = "Project items list"
    ProjectManagersList = "Project managers list"
    ProjectItemsLoaded = "Project items loaded"
    ProjectItemsSaved = "Project items saved"
    ShowAlarmServices = "Show alarm services"
    ShowConfiguration = "Show configuration"
    ShowDevice = "Show device"
    ShowNavigationItem = "Show navigation item"


class KaraboBroadcastEvent(QEvent):
    """ Custom event to handle GUI widget communication """

    # Ask Qt for our event type
    Type = QEvent.Type(QEvent.registerEventType())

    def __init__(self, sender="", data=None):
        super(KaraboBroadcastEvent, self).__init__(self.Type)
        self.sender = sender  # Names the sender
        self.data = data or {}  # Includes the data which is sent


def broadcast_event(event):
    """ Broadcast the given `event`.
    """
    mediator = get_mediator()
    QApplication.postEvent(mediator, event)


def register_for_broadcasts(qobject):
    """ Register the given `qobject` to the events coming from the singleton
        `mediator`.
    """
    mediator = get_mediator()
    mediator.installEventFilter(qobject)


def unregister_from_broadcasts(qobject):
    """ Unregister the given `qobject` from the events coming from the
        singleton mediator object.
    """
    mediator = get_mediator()
    mediator.removeEventFilter(qobject)
