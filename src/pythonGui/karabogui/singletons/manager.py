#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import defaultdict
from datetime import datetime
from functools import wraps

from PyQt4.QtCore import pyqtSlot, QObject
from PyQt4.QtGui import QMessageBox

from karabo.common.api import State, DeviceStatus
from karabo.native import AccessMode, Hash
from karabogui.alarms.api import extract_alarms_data
from karabogui.background import executeLater, Priority
from karabogui.binding.api import (
    apply_configuration, extract_attribute_modifications, extract_configuration
)
from karabogui.events import broadcast_event, KaraboEventSender
from karabogui import messagebox
from karabogui.singletons.api import get_network, get_topology


def handle_device_state_change(proxy, value):
    """This forwards a device descriptor's state to the configuration panel

    XXX: This might not be needed any longer.
    """
    data = {'configuration': proxy,
            'is_changing': State(value).isDerivedFrom(State.CHANGING)}
    broadcast_event(KaraboEventSender.DeviceStateChanged, data)


def project_db_handler(fall_through=False):
    """Decorator for project DB handlers to cut down on boilerplate"""

    def inner(handler):
        @wraps(handler)
        def wrapped(self, reply):
            success = reply.get('success', True)
            if not success:
                # NOTE: Don't block the event loop with these messages!
                messagebox.show_error(reply['reason'], modal=False)
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
        schema = self._topology.get_schema(serverId, classId)

        # if we do not have a schema, notify the user and return
        if schema is None:
            msg = ('Please install device plugin {} on server {} '
                   'first.'.format(classId, serverId))
            messagebox.show_warning(msg, title='Unknown device schema')
            return

        # Use standard configuration for server/classId
        class_proxy = self._topology.get_class(serverId, classId)
        if config is None:
            config = extract_configuration(class_proxy.binding)

        def _walk_config(config, base=''):
            base = base + '.' if base else ''
            for key, value, _ in config.iterall():
                subkey = base + key
                if isinstance(value, Hash):
                    yield from _walk_config(value, base=subkey)
                else:
                    yield subkey

        # XXX: Temporary fix - due to the state changes
        # Old projects save all parameters, even invalid ones. This fix
        # removes them from the initial configuration to not stop the validator
        # from instantiating
        obsolete_paths = [pth for pth in _walk_config(config)
                          if pth not in schema.hash]
        for key in obsolete_paths:
            config.erase(key)

        readonly_paths = [pth for pth in _walk_config(config)
                          if schema.hash[pth, "accessMode"] ==
                          AccessMode.READONLY.value]
        for key in readonly_paths:
            config.erase(key)

        # Compute a runtime schema from the project device proxy and an
        # unmodified copy of the device class schema.
        attr_updates = None
        project_dev_proxy = self._topology.get_project_device_proxy(
            deviceId, serverId, classId, create_instance=False)
        if project_dev_proxy is not None:
            # This feature only works with named devices!
            attr_updates = extract_attribute_modifications(
                schema, project_dev_proxy.binding)

        # Send signal to network
        get_network().onInitDevice(serverId, classId, deviceId, config,
                                   attrUpdates=attr_updates)

    def callDeviceSlot(self, token, handler, device_id, slot_name, params):
        """Call a device slot using the `requestFromSlot` mechanism.

        See karabogui.request for more details.
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

    @pyqtSlot(object)
    def onReceivedData(self, hsh):
        handler = getattr(self, "handle_" + hsh["type"])
        kwargs = {k: v for k, v in hsh.items() if k != "type"}
        handler(**kwargs)

    @pyqtSlot(bool)
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

    def handle_configurationFromPast(self, **info):
        success = info.get('success', False)
        if not success:
            reason = info.get('reason')
            messagebox.show_error(reason, modal=False)
            return
        deviceId = info.get('deviceId')
        config = info.get('config')
        broadcast_event(KaraboEventSender.ShowConfigurationFromPast,
                        {'deviceId': deviceId, 'configuration': config})

    def handle_brokerInformation(self, **info):
        get_network()._handleBrokerInformation(
            info.get('host'), info.get('port'), info.get('topic'))
        broadcast_event(KaraboEventSender.brokerInformationUpdate, info)

    def handle_systemTopology(self, systemTopology):
        self._topology.update(systemTopology)

        # Tell the GUI about various devices that are alive
        instance_ids = self._collect_devices('AlarmService')
        self._announce_alarm_services(instance_ids.get('AlarmService', []))

        # Tell the world about new devices/servers
        devices, servers = _extract_topology_devices(systemTopology)
        broadcast_event(KaraboEventSender.SystemTopologyUpdate,
                        {'devices': devices, 'servers': servers})

    def handle_systemVersion(self, **info):
        """Handle the version number reply from the GUI server"""
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
        for instance_id, class_id, _ in devices:
            if class_id == 'AlarmService':
                self._announce_alarm_services([instance_id])
            elif class_id == 'ProjectManager':
                broadcast_event(KaraboEventSender.ProjectDBConnect,
                                {'device': instance_id})

    def handle_instanceUpdated(self, topologyEntry):
        self._topology.instance_updated(topologyEntry)

    def handle_instanceGone(self, instanceId, instanceType):
        """Remove ``instance_id`` from topology and update
        """
        # Tell the GUI about various devices that are now gone
        self._broadcast_if_of_type('AlarmService', instanceId,
                                   KaraboEventSender.RemoveAlarmServices)

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
        self._topology.class_schema_updated(serverId, classId, schema)

    def handle_deviceSchema(self, deviceId, schema):
        proxy = self._topology.device_schema_updated(deviceId, schema)
        if proxy is None:
            return

        # XXX: Do we really need this?
        # Listen to the Configuration object's state changes
        # handler = partial(handle_device_state_change, proxy)
        # proxy.state_binding.on_trait_change(handler, 'value')

        # Refresh the configurator iff this proxy is already showing
        broadcast_event(KaraboEventSender.UpdateDeviceConfigurator,
                        {'proxy': proxy})

    def handle_deviceConfiguration(self, deviceId, configuration):
        self._topology.device_config_updated(deviceId, configuration)

    def handle_propertyHistory(self, deviceId, property, data):
        device_proxy = self._topology.get_device(deviceId)
        device_proxy.publish_historic_data(property, data)

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

    @project_db_handler(fall_through=True)
    def handle_projectSaveItems(self, reply):
        # ``reply`` is a Hash containing a list of item hashes
        success = reply.get('success', True)
        d = {'success': success, 'items': reply.get('items', [])}
        broadcast_event(KaraboEventSender.ProjectItemsSaved, d)

    @project_db_handler()
    def handle_projectUpdateAttribute(self, reply):
        # ``reply`` is a Hash containing a list of item hashes
        broadcast_event(KaraboEventSender.ProjectAttributeUpdated,
                        {'items': reply['items']})

    # ---------------------------------------------------------------------

    def handle_notification(self, device, message, short, detailed):
        pass  # DEPRECATED

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
            device_proxy = self._topology.get_device(device_id)
            binding = device_proxy.get_property_binding(prop_path)
            apply_configuration(data_hash, binding.value.schema)
            device_proxy.config_update = True
            # Let the GUI server know we have processed this chunk
            get_network().onRequestNetwork(name)

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
            data = extract_alarms_data(instanceId, rows)
            self._topology.update_alarms_info(data)

            broadcast_event(KaraboEventSender.AlarmServiceInit, data)

    def handle_alarmUpdate(self, instanceId, rows):
        """Show update for ``AlarmService`` with given ``instanceId`` and all
           the information given in the ``Hash`` ``rows``.
        """
        if not rows.empty():
            data = extract_alarms_data(instanceId, rows)
            self._topology.update_alarms_info(data)

            broadcast_event(KaraboEventSender.AlarmServiceUpdate, data)

    def handle_runConfigSourcesInGroup(self, reply):
        pass  # DEPRECATED

    # ------------------------------------------------------------------
    # Private methods

    def _announce_alarm_services(self, instance_ids):
        """Handle the arrival of one or more `AlarmService` devices.
        """
        if instance_ids:
            broadcast_event(KaraboEventSender.ShowAlarmServices,
                            {'instanceIds': instance_ids})
        for inst_id in instance_ids:
            # Request all current alarms for the given alarm service device
            get_network().onRequestAlarms(inst_id)

    def _broadcast_if_of_type(self, class_id, instance_id, event_type):
        """If a device is of a particular type `class_id`, broadcast an event.
        """
        attrs = self._topology.get_attributes('device.' + instance_id)
        if attrs is not None:
            if class_id == attrs.get('classId', ''):
                broadcast_event(event_type, {'instanceIds': [instance_id]})
                return True
        return False

    def _collect_devices(self, *class_ids):
        """Walk the system tree and collect all the instance ids of devices
        whose type is in the list `class_ids`.
        """
        instance_ids = defaultdict(list)

        def visitor(node):
            attrs = node.attributes
            dev_class_id = attrs.get('classId', 'UNKNOWN')
            if attrs.get('type') == 'device' and dev_class_id in class_ids:
                instance_ids[dev_class_id].append(node.node_id)

        self._topology.visit_system_tree(visitor)
        return instance_ids

# ------------------------------------------------------------------


def _extract_topology_devices(topo_hash):
    """Get all the devices and their classes out of a system topology update.
    """
    devices, servers = [], []

    if 'device' in topo_hash:
        for device_id, _, attrs in topo_hash['device'].iterall():
            class_id = attrs.get('classId', 'unknown-class')
            status = attrs.get('status', 'ok')
            devices.append((device_id, class_id, DeviceStatus(status)))

    if 'macro' in topo_hash:
        for device_id, _, attrs in topo_hash['macro'].iterall():
            class_id = attrs.get('classId', 'unknown-class')
            devices.append((device_id, class_id, DeviceStatus.OK))

    if 'server' in topo_hash:
        for server_id, _, attrs in topo_hash['server'].iterall():
            host = attrs.get('host', 'UNKNOWN')
            status = attrs.get('status', 'ok')
            servers.append((server_id, host, DeviceStatus(status)))

    return devices, servers
