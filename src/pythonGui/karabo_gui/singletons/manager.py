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
from functools import wraps

from PyQt4.QtCore import QObject
from PyQt4.QtGui import QMessageBox

from karabo.common.api import State
from karabo_gui.background import executeLater, Priority
from karabo_gui.events import broadcast_event, KaraboEventSender
from karabo_gui.singletons.api import get_network, get_topology
from karabo_gui.util import getSchemaAttributeUpdates


def handle_device_state_change(box, value, timestamp):
    """This forwards a device descriptor's state to the configuration panel
    """
    configuration = box.configuration

    if State(value).isDerivedFrom(State.CHANGING):
        # Only a state change
        data = {'configuration': configuration, 'is_changing': True}
        broadcast_event(KaraboEventSender.DeviceStateChanged, data)
    else:
        # First the error state
        data = {'configuration': configuration,
                'is_changing': State(value) is State.ERROR}
        broadcast_event(KaraboEventSender.DeviceErrorChanged, data)

        # Then a regular state change notification
        data = {'configuration': configuration,
                'is_changing': False}
        broadcast_event(KaraboEventSender.DeviceStateChanged, data)


def project_db_handler(fall_through=False):
    """Decorator for project DB handlers to cut down on boilerplate"""

    def inner(handler):
        @wraps(handler)
        def wrapped(self, reply):
            success = reply.get('success', True)
            if not success:
                QMessageBox.critical(None, 'Error', reply['reason'])
            # If needed, call the handler even when there was a failure
            if fall_through or success:
                return handler(self, reply)

        return wrapped
    return inner


class Manager(QObject):
    def __init__(self, parent=None):
        super(Manager, self).__init__(parent=parent)

        # The system topology singleton
        self._topology = get_topology()
        # A dict which includes big networkData
        self._big_data = {}
        # Handler callbacks for `handle_requestFromSlot`
        self._request_handlers = {}

        network = get_network()
        network.signalServerConnectionChanged.connect(
            self.onServerConnectionChanged)
        network.signalReceivedData.connect(self.onReceivedData)

    def initDevice(self, serverId, classId, deviceId, config=None):
        baseSchema = self._topology.get_schema(serverId, classId)

        # if we do not have a schema, notify the user and return
        if baseSchema is None:
            QMessageBox.warning(None, 'Unknown device schema',
                                'Please install device plugin {} on '
                                'server {} first.'.format(classId, serverId))
            return

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
        schemaAttrUpdates = getSchemaAttributeUpdates(baseSchema, config)

        # Send signal to network
        get_network().onInitDevice(serverId, classId, deviceId, config,
                                   attrUpdates=schemaAttrUpdates)

    def callDeviceSlot(self, token, handler, device_id, slot_name, params):
        """Call a device slot using the `requestFromSlot` mechanism.

        See karabo_gui.request for more details.
        """
        # Set the callback handler
        assert token not in self._request_handlers
        assert callable(handler)
        self._request_handlers[token] = handler

        # Call the GUI server
        get_network().onExecuteGeneric(token, device_id, slot_name, params)

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
        broadcast_event(KaraboEventSender.NetworkConnectStatus,
                        {'status': isConnected})

        # Clear the system topology
        if not isConnected:
            self._topology.clear()

            # Any pending requests are effectively timed-out
            pending = list(self._request_handlers.keys())
            for token in pending:
                self.handle_requestFromSlot(token, False, info=None)

    def handle_requestFromSlot(self, token, success, reply=None, info=None):
        handler = self._request_handlers.pop(token, lambda s, r: None)
        # If the request failed at the server level, `reply` is None and
        # `info` is passed instead
        info = info or {}
        hsh = info if reply is None or not success else reply

        # Wrap the handler invocation in an exception swallower
        try:
            handler(success, hsh)
        except Exception as ex:
            # But at least show something when exceptions are being swallowed
            msg = ('Exception of type "{}" caught in callback handler "{}" '
                   'with reply:\n{}')
            print(msg.format(type(ex).__name__, handler, hsh))

    def handle_log(self, messages):
        broadcast_event(KaraboEventSender.LogMessages, {'messages': messages})

    def handle_brokerInformation(self, **kwargs):
        get_network()._handleBrokerInformation(**kwargs)

    def handle_systemTopology(self, systemTopology):
        self._topology.update(systemTopology)

        # Tell the GUI about various devices that are alive
        self._check_for_alarm_service()
        self._broadcast_about_instances('RunConfigurator',
                                        KaraboEventSender.AddRunConfigurator)
        self._broadcast_about_instances('RunConfigurationGroup',
                                        KaraboEventSender.AddRunConfigGroup)

        # Tell the world about new devices/servers
        devices, servers = _extract_topology_devices(systemTopology)
        broadcast_event(KaraboEventSender.SystemTopologyUpdate,
                        {'devices': devices, 'servers': servers})

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
        broadcast_event(KaraboEventSender.SystemTopologyUpdate,
                        {'devices': devices, 'servers': servers})

        # Tell the GUI about various devices or servers that are alive
        self._check_for_alarm_service()
        self._broadcast_about_instances('RunConfigurator',
                                        KaraboEventSender.AddRunConfigurator)
        self._broadcast_about_instances('RunConfigurationGroup',
                                        KaraboEventSender.AddRunConfigGroup)

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
        self._broadcast_about_instances(
            'RunConfigurationGroup', KaraboEventSender.RemoveRunConfigGroup,
            transform=_instance_finder)

        # Update the system topology
        devices, servers = self._topology.instance_gone(instanceId,
                                                        instanceType)
        # Broadcast the change to listeners
        broadcast_event(KaraboEventSender.SystemTopologyUpdate,
                        {'devices': devices, 'servers': servers})

        if instanceType == 'server':
            # Clear corresponding parameter pages
            # TODO: We need to find all the Configuration objects in the open
            # projects and clear their parameter editor objects if they happen
            # to belong to the server that was just removed!
            # This was formerly handled by the old ProjectModel class
            pass

        # Once everything has calmed down, tell the configurator to clear
        # NOTE: Doing this last avoids resetting displayed project devices
        broadcast_event(KaraboEventSender.ClearConfigurator,
                        {'deviceId': instanceId})

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
        broadcast_event(KaraboEventSender.UpdateDeviceConfigurator,
                        {'configuration': conf})

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
        try:
            box = device.getBox(property.split("."))
        except AttributeError:
            # When a property is renamed, we get an AttributeError here.
            # Return and deal with the consequences in the layers above
            return
        box.signalHistoricData.emit(box, data)

    # ---------------------------------------------------------------------
    # Current Project Interface

    def handle_projectBeginUserSession(self, reply):
        pass

    def handle_projectEndUserSession(self, reply):
        pass

    @project_db_handler()
    def handle_projectListDomains(self, reply):
        # ``reply`` is a Hash containing a list of domain names
        broadcast_event(KaraboEventSender.ProjectDomainsList,
                        {'items': reply['domains']})

    @project_db_handler()
    def handle_projectListItems(self, reply):
        # ``reply`` is a Hash containing a list of item hashes
        broadcast_event(KaraboEventSender.ProjectItemsList,
                        {'items': reply.get('items', [])})

    def handle_projectListProjectManagers(self, reply):
        # ``reply`` is a list of strings
        broadcast_event(KaraboEventSender.ProjectManagersList,
                        {'items': reply})

    @project_db_handler(fall_through=True)
    def handle_projectLoadItems(self, reply):
        # ``reply`` is a Hash containing a list of item hashes
        success = reply.get('success', True)
        d = {'success': success, 'items': reply.get('items', [])}
        broadcast_event(KaraboEventSender.ProjectItemsLoaded, d)

    @project_db_handler()
    def handle_projectSaveItems(self, reply):
        # ``reply`` is a Hash containing a list of item hashes
        broadcast_event(KaraboEventSender.ProjectItemsSaved,
                        {'items': reply['items']})

    @project_db_handler()
    def handle_projectUpdateAttribute(self, reply):
        # ``reply`` is a Hash containing a list of item hashes
        broadcast_event(KaraboEventSender.ProjectAttributeUpdated,
                        {'items': reply['items']})

    # ---------------------------------------------------------------------

    def handle_notification(self, device, message, short, detailed):
        data = {'device_id': device,
                'message_type': message,
                'short_msg': short,
                'detailed_msg': detailed}
        broadcast_event(KaraboEventSender.NotificationMessage, data)

    def handle_networkData(self, name, data):
        """This method handles the big data chucks coming from directly
        connected devices (p2p) to `GuiServerDevice`. To keep the GUI
        responsive the displaying of this data is delayed here.
        """
        def show_data():
            if name not in self._big_data:
                return
            data_hash = self._big_data.pop(name)
            device_id, prop_path = name.split(":")
            device = self._topology.get_device(device_id)
            box = device.getBox(prop_path.split("."))
            box.boxvalue.schema.fromHash(data_hash)

        self._big_data[name] = data
        executeLater(show_data, Priority.BIG_DATA)

    def handle_initReply(self, deviceId, success, message):
        device = self._topology.get_device(deviceId)
        if device is not None:
            data = {'device': device, 'success': success, 'message': message}
            # Create KaraboBroadcastEvent
            broadcast_event(KaraboEventSender.DeviceInitReply, data)

    def handle_alarmInit(self, instanceId, rows):
        """Show initial update for ``AlarmService`` with given ``instanceId``
           and all the information given in the ``Hash`` ``rows``.
        """
        if not rows.empty():
            # Create KaraboBroadcastEvent only if there is something to show
            broadcast_event(KaraboEventSender.AlarmInitReply,
                            {'instanceId': instanceId, 'rows': rows})

    def handle_alarmUpdate(self, instanceId, rows):
        """Show update for ``AlarmService`` with given ``instanceId`` and all
           the information given in the ``Hash`` ``rows``.
        """
        if rows.empty():
            return

        # Create KaraboBroadcastEvent only if there is something to show
        broadcast_event(KaraboEventSender.AlarmUpdate,
                        {'instanceId': instanceId, 'rows': rows})

        for hsh in rows.values():
            # Get data of hash
            for aHash in hsh.values():
                # Fetch only deviceId and type to broadcast this
                data = {'deviceId': aHash.get('deviceId'),
                        'alarm_type': aHash.get('type')}
                broadcast_event(KaraboEventSender.AlarmDeviceUpdate, data)

    def handle_runConfigSourcesInGroup(self, reply):
        broadcast_event(KaraboEventSender.RunConfigSourcesUpdate, reply)

    # ------------------------------------------------------------------
    # Private methods

    def _broadcast_about_instances(self, class_id, event_type, transform=None):
        """Search the system topology for devices of a given `class_id` and
        then (possibly) transform that list of device IDs with a `transform`
        function. If the transformed list has a length > 0, broadcast an event
        containing the remaining IDs.
        """
        instance_ids = set()

        def visitor(node):
            attrs = node.attributes
            dev_class_id = attrs.get('classId', 'UNKNOWN')
            if attrs.get('type') == 'device' and dev_class_id == class_id:
                instance_ids.add(node.node_id)

        self._topology.visit_system_tree(visitor)
        instance_ids = list(instance_ids)
        if transform is not None:
            transform(instance_ids)
        if instance_ids:
            # Tell the world
            broadcast_event(event_type, {'instanceIds': instance_ids})

        return instance_ids

    def _check_for_alarm_service(self):
        """Fetch all available ``AlarmService`` device instance ids and trigger
        request for those devices to network
        """
        instance_ids = self._broadcast_about_instances(
            'AlarmService', KaraboEventSender.ShowAlarmServices)
        for inst_id in instance_ids:
            # Request all current alarms for the given alarm service device
            get_network().onRequestAlarms(inst_id)

    def _device_data_received(self):
        """Notify all listeners that some (class, schema, or config) data was
        received.
        """
        broadcast_event(KaraboEventSender.DeviceDataReceived, {})


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
