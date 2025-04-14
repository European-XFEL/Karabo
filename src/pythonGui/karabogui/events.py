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
from enum import Enum

from qtpy.QtCore import QEvent
from qtpy.QtWidgets import QApplication

from karabogui.singletons.api import get_mediator


# Enum for karabo broadcast event senders
class KaraboEvent(Enum):
    AccessLevelChanged = "Global Access Level changed"
    ActiveDestinations = "Active Proposal (Logbook) list "
    BigDataProcessing = "Big data delay"
    ClearConfigurator = "Clear configurator"
    ConnectMacroInstance = "Connect macro instance"
    CreateMainWindow = "Create the main window"
    CustomEvent = "Custom Event"  # Generic event
    DatabaseIsBusy = "Database is busy"
    DeviceInitReply = "Device instantiate reply"
    DeviceStateChanged = "Device state changed"
    ListConfigurationUpdated = "Configuration List Updated"
    LoadConfiguration = "Load device configuration"
    LoginUserChanged = "User changed"
    LogMessages = "Log messages"
    MaximizePanel = "Maximize a panel"
    MinimizePanel = "Minimize a panel"
    MiddlePanelClosed = "Middle panel closed"
    NetworkConnectStatus = "Network connection changed"
    OpenSceneLink = "Scene link"
    RemoveAlarmServices = "Remove alarm services"
    RemoveProjectModelViews = "Remove scene or macro views"
    ProjectName = "Update main window project name"
    ProjectDBConnect = "Reestablish connection to project db"
    ProjectDomainsList = "Project domains list"
    ProjectAttributeUpdated = "Project attribute updated"
    ProjectFilterUpdated = "Project filter updated"
    ProjectItemsList = "Project items list"
    ProjectItemsLoaded = "Project items loaded"
    ProjectItemsSaved = "Project items saved"
    ProjectManagersList = "Project managers list"
    ProjectUpdated = "Project Updated"
    ProjectFindWithDevice = "Find projects with device"
    RaiseEditor = "Raise the Editor"
    RestoreSceneView = "Restore the scene"
    ServerInformationUpdate = "Server information update"
    ServerNotification = "Server Notification"
    ShowConfiguration = "Show configuration"
    ShowConfigurationFromName = "Show Configuration From Name"
    ShowConfigurationFromPast = "Show Configuration From Past"
    ShowDevice = "Show device"
    ShowProjectDevice = "Show project device"
    ShowProjectModel = "Show project model"
    ShowMacroView = "Show macro"
    ShowSceneView = "Show scene"
    ShowUnattachedController = "Show unattached controller"
    ShowUnattachedSceneView = "Show unattached scene"
    StartMonitoringDevice = "Start listening to device updates"
    StopMonitoringDevice = "Stop listening to device updates"
    SystemTopologyUpdate = "System topology updated"
    TemporarySession = "Temporary session"
    UpdateDeviceConfigurator = "Update device in configurator"
    UpdateValueConfigurator = "Value update in configurator"


class KaraboBroadcastEvent(QEvent):
    """ Custom event to handle GUI widget communication """

    # Ask Qt for our event type
    Type = QEvent.Type(QEvent.registerEventType())

    def __init__(self, sender="", data=None):
        super().__init__(self.Type)
        self.sender = sender  # Names the sender
        self.data = data or {}  # Includes the data which is sent


def broadcast_event(sender_enum, data):
    """ Broadcast the given `event`.
    """
    assert isinstance(sender_enum, KaraboEvent)

    mediator = get_mediator()
    event = KaraboBroadcastEvent(sender=sender_enum, data=data)
    QApplication.postEvent(mediator, event)


def register_for_broadcasts(event_map):
    """ Register the given `qobject` to the events coming from the singleton
        `mediator`.
    """
    get_mediator().register_listener(event_map)


def unregister_from_broadcasts(event_map):
    """ Unregister the given `qobject` from the events coming from the
        singleton mediator object.
    """
    get_mediator().unregister_listener(event_map)
