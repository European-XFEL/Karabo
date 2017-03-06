from enum import Enum

from PyQt4.QtCore import QEvent
from PyQt4.QtGui import QApplication

from karabo_gui.singletons.api import get_mediator


# Enum for karabo broadcast event senders
class KaraboEventSender(Enum):
    AccessLevelChanged = "Global Access Level changed"
    AddRunConfigurator = "Add run configurator"
    AlarmInitReply = "Alarm init reply"
    AlarmUpdate = "Alarm update"
    AlarmDeviceUpdate = "Alarm device update"
    ClearConfigurator = "Clear configurator"
    ConnectMacroInstance = "Connect macro instance"
    CreateMainWindow = "Create the main window"
    DatabaseIsBusy = "Database is busy"
    DeviceDataReceived = "Device class/schema/config received"
    DeviceInitReply = "Device instantiate reply"
    DeviceErrorChanged = "Device error state changed"
    DeviceStateChanged = "Device state changed"
    LoginUserChanged = "User changed"
    LogMessages = "Log messages"
    MaximizePanel = "Maximize a panel"
    MinimizePanel = "Minimize a panel"
    MiddlePanelClosed = "Middle panel closed"
    NetworkConnectStatus = "Network connection changed"
    NotificationMessage = "Notification message"
    OpenMacro = "Open macro"
    OpenSceneLink = "Scene link"
    OpenSceneView = "Open scene view"
    RemoveAlarmServices = "Remove alarm services"
    RemoveRunConfigurator = "Remove run configurator"
    RemoveMacro = "Remove macro"
    RemoveSceneView = "Remove scene view"
    ProjectDomainsList = "Project domains list"
    ProjectItemsList = "Project items list"
    ProjectItemsLoaded = "Project items loaded"
    ProjectItemsSaved = "Project items saved"
    ProjectManagersList = "Project managers list"
    RunConfigSourcesUpdate = "Run config sources in group received"
    ShowAlarmServices = "Show alarm services"
    ShowConfiguration = "Show configuration"
    ShowDevice = "Show device"
    ShowNavigationItem = "Show navigation item"
    StartMonitoringDevice = "Start listening to device updates"
    StopMonitoringDevice = "Stop listening to device updates"
    SystemTopologyUpdate = "System topology updated"
    UpdateDeviceConfigurator = "Update device in configurator"


class KaraboBroadcastEvent(QEvent):
    """ Custom event to handle GUI widget communication """

    # Ask Qt for our event type
    Type = QEvent.Type(QEvent.registerEventType())

    def __init__(self, sender="", data=None):
        super(KaraboBroadcastEvent, self).__init__(self.Type)
        self.sender = sender  # Names the sender
        self.data = data or {}  # Includes the data which is sent


def broadcast_event(sender_enum, data):
    """ Broadcast the given `event`.
    """
    assert isinstance(sender_enum, KaraboEventSender)

    mediator = get_mediator()
    event = KaraboBroadcastEvent(sender=sender_enum, data=data)
    QApplication.postEvent(mediator, event)


def register_for_broadcasts(qobject):
    """ Register the given `qobject` to the events coming from the singleton
        `mediator`.
    """
    get_mediator().register_listener(qobject)


def unregister_from_broadcasts(qobject):
    """ Unregister the given `qobject` from the events coming from the
        singleton mediator object.
    """
    get_mediator().unregister_listener(qobject)
