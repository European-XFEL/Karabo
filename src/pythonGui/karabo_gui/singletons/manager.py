#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
"""This module contains the manager class which works as a man in the middle of
   the star structure. All relevant signals go over here.

   The manager class is a singleton.

   All relevant configuration data is stored in a member hash variable.
"""

from datetime import datetime

from PyQt4.QtCore import QObject
from PyQt4.QtGui import QMessageBox

from karabo.common.states import State
from karabo_gui.events import (
    broadcast_event, KaraboBroadcastEvent, KaraboEventSender)
from karabo_gui.messagebox import MessageBox
from karabo_gui.singletons.api import get_network, get_topology
from karabo_gui.util import getSchemaAttributeUpdates


def handle_device_state_change(box, value, timestamp):
    """This forwards a device descriptor's state to the configuration panel
    """
    broadcast = broadcast_event
    configuration = box.configuration

    if State(value).isDerivedFrom(State.CHANGING):
        # Only a state change
        data = {'configuration': configuration, 'is_changing': True}
        broadcast(KaraboBroadcastEvent(KaraboEventSender.DeviceStateChanged,
                                       data))
    else:
        # First the error state
        data = {'configuration': configuration,
                'is_changing': State(value) is State.ERROR}
        broadcast(KaraboBroadcastEvent(KaraboEventSender.DeviceErrorChanged,
                                       data))

        # Then a regular state change notification
        data = {'configuration': configuration,
                'is_changing': False}
        broadcast(KaraboBroadcastEvent(KaraboEventSender.DeviceStateChanged,
                                       data))


class Manager(QObject):
    def __init__(self, parent=None):
        super(Manager, self).__init__(parent=parent)

        # The system topology singleton
        self._topology = get_topology()

        network = get_network()
        network.signalServerConnectionChanged.connect(
            self.onServerConnectionChanged)
        network.signalReceivedData.connect(self.onReceivedData)

    def initDevice(self, serverId, classId, deviceId, config=None):
        # Use standard configuration for server/classId
        conf = self._topology.get_class(serverId, classId)
        if config is None:
            config, _ = conf.toHash()  # Ignore returned attributes

        # XXX: Temporary fix - due to the state changes
        # Old projects save all parameters, even the read only ones. This fix
        # removes them from the initial configuration to not stop the validator
        # from instantiating
        descriptor = conf.descriptor
        if descriptor is not None:
            obsolete_paths = descriptor.getObsoletePaths(config)
            for key in obsolete_paths:
                config.erase(key)
            read_only_paths = descriptor.getReadOnlyPaths()
            for key in read_only_paths:
                # Remove all read only parameters
                if key in config:  # erase does not tolerate non-existing keys
                    config.erase(key)

        # Compute a runtime schema from the configuration and an unmodified
        # copy of the device class schema.
        baseSchema = self._topology.get_schema(serverId, classId)
        schemaAttrUpdates = getSchemaAttributeUpdates(baseSchema, config)

        # Send signal to network
        get_network().onInitDevice(serverId, classId, deviceId, config,
                                   attrUpdates=schemaAttrUpdates)

    def shutdownDevice(self, deviceId, showConfirm=True):
        if showConfirm:
            ask = ('Do you really want to shutdown the device '
                   '"<b>{}</b>"?').format(deviceId)
            reply = QMessageBox.question(None, 'Shutdown device', ask,
                                         QMessageBox.Yes | QMessageBox.No,
                                         QMessageBox.No)
            if reply == QMessageBox.No:
                return

        get_network().onKillDevice(deviceId)

    def shutdownServer(self, serverId):
        ask = ('Do you really want to shutdown the server '
               '"<b>{}</b>"?').format(serverId)
        reply = QMessageBox.question(None, 'Shutdown server', ask,
                                     QMessageBox.Yes | QMessageBox.No,
                                     QMessageBox.No)
        if reply == QMessageBox.No:
            return

        get_network().onKillServer(serverId)

    def onReceivedData(self, hash):
        getattr(self, "handle_" + hash["type"])(
            **{k: v for k, v in hash.items() if k != "type"})

    def onServerConnectionChanged(self, isConnected):
        """If the server connection is changed, the model needs an update.
        """
        # Broadcast event to all panels which need a reset
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.NetworkConnectStatus, {'status': isConnected}))

        # Clear the system topology
        if not isConnected:
            self._topology.clear()

    def handle_log(self, messages):
        data = {'messages': messages}
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.LogMessages, data))

    def handle_brokerInformation(self, **kwargs):
        get_network()._handleBrokerInformation(**kwargs)

    def handle_systemTopology(self, systemTopology):
        self._topology.update(systemTopology)

        # Tell the GUI about various devices that are alive
        self._broadcast_about_instances('AlarmService',
                                        KaraboEventSender.ShowAlarmServices)
        self._broadcast_about_instances('RunConfigurator',
                                        KaraboEventSender.AddRunConfigurator)

        # Tell the world about new devices/servers
        devices, servers = _extract_topology_devices(systemTopology)
        data = {'devices': devices, 'servers': servers}
        broadcast_event(KaraboBroadcastEvent(
                KaraboEventSender.SystemTopologyUpdate, data))

    def handle_systemVersion(self, version):
        """ Handle the version number reply from the GUI server.
        """
        pass

    def handle_instanceNew(self, topologyEntry):
        """This function receives the configuration for a new instance.

        If the instance already exists in the central hash, it is first
        removed from there.
        """
        existing_devices = self._topology.instance_new(topologyEntry)

        log_messages = []
        for inst_id in existing_devices:
            message = {
                'timestamp': datetime.now().isoformat(),
                'type': 'INFO',
                'category': inst_id,
                'message': ('Detected dirty shutdown for instance "{}", '
                            'which is coming up now.').format(inst_id)
            }
            log_messages.append(message)
        self.handle_log(log_messages)

        devices, servers = _extract_topology_devices(topologyEntry)
        # Broadcast the change to listeners
        data = {'devices': devices, 'servers': servers}
        broadcast_event(KaraboBroadcastEvent(
                        KaraboEventSender.SystemTopologyUpdate, data))

        # Tell the GUI about various devices or servers that are alive
        self._broadcast_about_instances('AlarmService',
                                        KaraboEventSender.ShowAlarmServices)
        self._broadcast_about_instances('RunConfigurator',
                                        KaraboEventSender.AddRunConfigurator)

    def handle_instanceUpdated(self, topologyEntry):
        self._topology.instance_updated(topologyEntry)

    def handle_instanceGone(self, instanceId, instanceType):
        """ Remove ``instance_id`` from topology and update
        """
        def _instance_finder(instance_ids):
            """If `instanceId` is in the list, make it the only item.
            Otherwise, make sure the list is empty (so nothing will be
            broadcast).
            """
            if instanceId in instance_ids:
                instance_ids.clear()
                instance_ids.append(instanceId)
            else:
                instance_ids.clear()

        # Tell the GUI about various devices that are now gone
        self._broadcast_about_instances(
            'AlarmService', KaraboEventSender.RemoveAlarmServices,
            transform=_instance_finder)
        self._broadcast_about_instances(
            'RunConfigurator', KaraboEventSender.RemoveRunConfigurator,
            transform=_instance_finder)

        # Update the system topology
        devices, servers = self._topology.instance_gone(instanceId,
                                                        instanceType)
        # Broadcast the change to listeners
        data = {'devices': devices, 'servers': servers}
        broadcast_event(KaraboBroadcastEvent(
                KaraboEventSender.SystemTopologyUpdate, data))

        if instanceType == 'server':
            # Clear corresponding parameter pages
            # TODO: We need to find all the Configuration objects in the open
            # projects and clear their parameter editor objects if they happen
            # to belong to the server that was just removed!
            # This was formerly handled by the old ProjectModel class
            pass

        # Once everything has calmed down, tell the configurator to clear
        # NOTE: Doing this last avoids resetting displayed project devices
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.ClearConfigurator, {'deviceId': instanceId}))

    def handle_attributesUpdated(self, reply):
        instanceId = reply["instanceId"]
        schema = reply["updatedSchema"]
        self.handle_deviceSchema(instanceId, schema)

    def handle_classSchema(self, serverId, classId, schema):
        conf = self._topology.class_schema_updated(serverId, classId, schema)
        if conf is None:
            return

        # Trigger update scenes
        self._device_data_received()

    def handle_deviceSchema(self, deviceId, schema):
        conf = self._topology.device_schema_updated(deviceId, schema)
        if conf is None:
            return

        # Listen to the Configuration object's state changes
        box_signal = conf.boxvalue.state.signalUpdateComponent
        box_signal.connect(handle_device_state_change)

        # Refresh the configurator iff this configuration is already showing
        data = {'configuration': conf}
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.UpdateDeviceConfigurator, data))

        # Trigger update scenes
        self._device_data_received()

    def handle_deviceConfiguration(self, deviceId, configuration):
        device = self._topology.device_config_updated(deviceId, configuration)
        if device is None:
            return

        if device.status in ('alive', 'monitoring'):
            # Trigger update scenes - to draw possible Workflow Connections
            self._device_data_received()

    def handle_propertyHistory(self, deviceId, property, data):
        device = self._topology.get_device(deviceId)
        box = device.getBox(property.split("."))
        box.signalHistoricData.emit(box, data)

    # ---------------------------------------------------------------------
    # Current Project Interface

    def handle_projectBeginUserSession(self, reply):
        pass

    def handle_projectEndUserSession(self, reply):
        pass

    def handle_projectListDomains(self, reply):
        # ``reply`` is a Hash containing a list of domain names
        d = {'items': reply['domains']}
        event = KaraboBroadcastEvent(KaraboEventSender.ProjectDomainsList, d)
        broadcast_event(event)

    def handle_projectListItems(self, reply):
        # ``reply`` is a Hash containing a list of item hashes
        success = reply.get('success', True)
        if not success:
            MessageBox.showError(reply['reason'])
            return
        d = {'items': reply['items']}
        event = KaraboBroadcastEvent(KaraboEventSender.ProjectItemsList, d)
        broadcast_event(event)

    def handle_projectListProjectManagers(self, reply):
        # ``reply`` is a list of strings
        d = {'items': reply}
        event = KaraboBroadcastEvent(KaraboEventSender.ProjectManagersList, d)
        broadcast_event(event)

    def handle_projectGetVersionInfo(self, reply):
        # ``reply`` is a Hash of uuid -> version info
        pass

    def handle_projectLoadItems(self, reply):
        # ``reply`` is a Hash containing a list of item hashes
        success = reply.get('success', True)
        if not success:
            MessageBox.showError(reply['reason'])
        d = {'success': success, 'items': reply['items']}  # Hash -> dict
        event = KaraboBroadcastEvent(KaraboEventSender.ProjectItemsLoaded, d)
        broadcast_event(event)

    def handle_projectSaveItems(self, reply):
        # ``reply`` is a Hash containing a list of item hashes
        d = {'items': reply['items']}  # Hash -> dict
        event = KaraboBroadcastEvent(KaraboEventSender.ProjectItemsSaved, d)
        broadcast_event(event)

    # ---------------------------------------------------------------------
    # Legacy Project Interface

    def handle_availableProjects(self, availableProjects):
        """ exists for compatibility with old GuiServers only """
        pass

    def handle_projectNew(self, name, success, data):
        """ exists for compatibility with old GuiServers only """
        pass

    def handle_projectLoaded(self, name, metaData, buffer):
        """ exists for compatibility with old GuiServers only """
        pass

    def handle_projectSaved(self, name, success, data):
        """ exists for compatibility with old GuiServers only """
        pass

    def handle_projectClosed(self, name, success, data):
        """ exists for compatibility with old GuiServers only """
        pass

    # ---------------------------------------------------------------------

    def handle_notification(self, device, message, short, detailed):
        data = {'device_id': device,
                'message_type': message,
                'short_msg': short,
                'detailed_msg': detailed}
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.NotificationMessage, data))

    def handle_networkData(self, name, data):
        device_id, prop_path = name.split(":")
        device = self._topology.get_device(device_id)
        box = device.getBox(prop_path.split("."))
        box.boxvalue.schema.fromHash(data)

    def handle_initReply(self, deviceId, success, message):
        device = self._topology.get_device(deviceId)
        if device is not None:
            data = {'device': device, 'success': success, 'message': message}
            # Create KaraboBroadcastEvent
            broadcast_event(KaraboBroadcastEvent(
                KaraboEventSender.DeviceInitReply, data))

    def handle_alarmInit(self, instanceId, rows):
        data = {'instanceId': instanceId, 'rows': rows}
        # Create KaraboBroadcastEvent
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.AlarmInitReply, data))

    def handle_alarmUpdate(self, instanceId, rows):
        data = {'instanceId': instanceId, 'rows': rows}
        # Create KaraboBroadcastEvent
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.AlarmUpdate, data))

        for hsh in rows.values():
            # Get data of hash
            for aHash in hsh.values():
                # Fetch only deviceId and type to broadcast this
                data = {'deviceId': aHash.get('deviceId'),
                        'alarm_type': aHash.get('type')}
                broadcast_event(KaraboBroadcastEvent(
                    KaraboEventSender.AlarmDeviceUpdate, data))

    def handle_runConfigSourcesInGroup(self, reply):
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.RunConfigSourcesUpdate, reply))

    # ------------------------------------------------------------------
    # Private methods

    def _broadcast_about_instances(self, class_id, event_type, transform=None):
        """Search the system topology for devices of a given `class_id` and
        then (possibly) transform that list of device IDs with a `transform`
        function. If the transformed list has a length > 0, broadcast an event
        containing the remaining IDs.
        """
        def filter(dev_id, attrs):
            dev_class_id = attrs.get('classId', 'UNKNOWN')
            return dev_class_id == class_id

        instance_ids = self._topology.search_system_tree('device', filter)
        if transform is not None:
            transform(instance_ids)
        if instance_ids:
            data = {'instanceIds': instance_ids}
            # Tell the world
            broadcast_event(KaraboBroadcastEvent(event_type, data))

    def _device_data_received(self):
        """Notify all listeners that some (class, schema, or config) data was
        received.
        """
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.DeviceDataReceived, {}))


# ------------------------------------------------------------------

def _extract_topology_devices(topo_hash):
    """Get all the devices and their classes out of a system topology update.
    """
    devices, servers = [], []

    if 'device' in topo_hash:
        for device_id, _, attrs in topo_hash['device'].iterall():
            class_id = attrs.get('classId', 'unknown-class')
            status = attrs.get('status', 'ok')
            devices.append((device_id, class_id, status))

    if 'macro' in topo_hash:
        for device_id, _, attrs in topo_hash['macro'].iterall():
            class_id = attrs.get('classId', 'unknown-class')
            devices.append((device_id, class_id, 'ok'))

    if 'server' in topo_hash:
        for server_id, _, attrs in topo_hash['server'].iterall():
            host = attrs.get('host', 'UNKNOWN')
            status = attrs.get('status', 'ok')
            servers.append((server_id, host, status))

    return devices, servers
