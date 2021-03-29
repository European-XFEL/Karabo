#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from datetime import datetime
from functools import wraps
from weakref import WeakKeyDictionary, WeakValueDictionary

from PyQt5.QtCore import pyqtSlot, QObject
from PyQt5.QtWidgets import QMessageBox

from karabo.common.api import ProxyStatus
from karabo.native import AccessMode, Assignment, Hash, Timestamp
from karabogui.alarms.api import extract_alarms_data
from karabogui.background import executeLater, Priority
from karabogui.binding.api import (
    apply_fast_data, extract_attribute_modifications, extract_configuration)
from karabogui.events import broadcast_event, KaraboEvent
from karabogui.globals import KARABO_CLIENT_ID
from karabogui import messagebox
from karabogui.singletons.api import get_alarm_model, get_network, get_topology
from karabogui.util import move_to_cursor, show_wait_cursor

from .util import get_error_message


def project_db_handler(fall_through=False):
    """Decorator for project DB handlers to cut down on boilerplate"""

    def inner(handler):
        @wraps(handler)
        def wrapped(self, reply):
            success = reply.get('success', True)
            if not success:
                messagebox.show_error(reply['reason'])
            # If needed, call the handler even when there was a failure
            if fall_through or success:
                return handler(self, reply)

        return wrapped

    return inner


class Manager(QObject):
    def __init__(self, parent=None):
        super(Manager, self).__init__(parent=parent)
        # this dictionary maps the deviceIds to a weak reference of the
        # DeviceProxy instances waiting for a reply from the server
        self._waiting_devices = WeakValueDictionary()
        # this dictionary maps a weak reference DeviceProxy with the list of
        # property proxies waiting to be heard
        self._waiting_properties = WeakKeyDictionary()
        # The system topology singleton
        self._topology = get_topology()
        self._alarm_model = get_alarm_model()
        # A dict which includes big networkData
        self._big_data = {}
        # Handler callbacks for `handle_requestGeneric`
        self._request_handlers = {}

        self._show_big_data_proc = False
        network = get_network()
        network.signalServerConnectionChanged.connect(
            self.onServerConnectionChanged)
        network.signalReceivedData.connect(self.onReceivedData)

    def toggleBigDataPerformanceMonitor(self):
        self._show_big_data_proc = True

    def expect_properties(self, device_proxy, properties):
        """Register the property proxies as waiting for replies"""
        device_id = device_proxy.device_id
        self._waiting_devices[device_id] = device_proxy
        proxies = self._waiting_properties.get(device_proxy, [])
        proxies.extend(properties)
        self._waiting_properties[device_proxy] = proxies

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

        # XXX: Temporary fix - due to the state changes
        # Old projects save all parameters, even invalid ones. This fix
        # removes them from the initial configuration to not stop the validator
        # from instantiating
        obsolete_paths = [pth for pth, _, _ in Hash.flat_iterall(config)
                          if pth not in schema.hash]
        for key in obsolete_paths:
            config.erase(key)

        readonly_paths = [pth for pth, _, _ in Hash.flat_iterall(config)
                          if schema.hash[pth, "accessMode"] ==
                          AccessMode.READONLY.value or
                          schema.hash[pth, "assignment"] ==
                          Assignment.INTERNAL.value]
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

    def callDeviceSlot(self, token, handler, instance_id, slot_name, params):
        """Call a device slot using the `requestGeneric` mechanism.

        See karabogui.request for more details.
        """
        # Set the callback handler
        assert token not in self._request_handlers
        assert callable(handler)
        self._request_handlers[token] = handler

        # Call the GUI server
        get_network().onExecuteGeneric(instance_id, slot_name, params)

    def shutdownDevice(self, deviceId, showConfirm=True, parent=None):
        if showConfirm:
            ask = ('Do you really want to shutdown the device '
                   '"<b>{}</b>"?').format(deviceId)
            msg_box = QMessageBox(QMessageBox.Question, 'Shutdown device', ask,
                                  QMessageBox.Yes | QMessageBox.No,
                                  parent=parent)

            msg_box.setModal(False)
            msg_box.setDefaultButton(QMessageBox.No)
            move_to_cursor(msg_box)
            if msg_box.exec() == QMessageBox.No:
                return

        get_network().onKillDevice(deviceId)

    def shutdownServer(self, serverId, parent=None):
        ask = ('Do you really want to shutdown the server '
               '"<b>{}</b>"?').format(serverId)
        msg_box = QMessageBox(QMessageBox.Question, 'Shutdown server', ask,
                              QMessageBox.Yes | QMessageBox.No, parent=parent)
        msg_box.setModal(False)
        msg_box.setDefaultButton(QMessageBox.No)
        move_to_cursor(msg_box)
        if msg_box.exec() == QMessageBox.No:
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
        broadcast_event(KaraboEvent.NetworkConnectStatus,
                        {'status': isConnected})

        # Clear the system topology
        if not isConnected:
            self._topology.clear()

            # Any pending requests are effectively timed-out
            pending = list(self._request_handlers.keys())
            for token in pending:
                self.handle_requestGeneric(
                    False, request=Hash("args.token", token),
                    reason="Karabo GUI Client disconnect. Erasing request.")

    def handle_reconfigureReply(self, **info):
        """Handle the reconfigure reply of the gui server"""
        success = info['success']
        input_info = info['input']
        deviceId = input_info['deviceId']
        if not success:
            reason = info.get('failureReason') or info.get('reason')
            # clear the waiting queue
            device = self._waiting_devices.pop(deviceId, None)
            props = self._waiting_properties.pop(device, [])
            paths = set(input_info['configuration'].paths())
            pending = [prop_proxy
                       for prop_proxy in props
                       if prop_proxy.path not in paths]
            if pending:
                # in case multiple updates are sent and only few failed
                self.expect_properties(device, pending)
            text = ('Device reconfiguration of <b>{}</b> encountered an error.'
                    ' The values could NOT be applied!'.format(deviceId))
            messagebox.show_error(text, details=reason)
        else:
            device_proxy = self._waiting_devices.pop(deviceId, None)
            if device_proxy is None:
                return
            prop_proxies = self._waiting_properties.pop(device_proxy, [])
            paths = set(input_info['configuration'].paths())
            pending = []
            for prop_proxy in prop_proxies:
                if prop_proxy.path not in paths:
                    pending.append(prop_proxy)
                    continue
                # revert the edit in the properties.
                prop_proxy.revert_edit()
            if pending:
                # in case multiple updates are sent, wait for the pending ones
                self.expect_properties(device_proxy, pending)
            broadcast_event(
                KaraboEvent.UpdateValueConfigurator,
                {'proxy': device_proxy})

    def handle_requestGeneric(self, success, request=None, reply=None,
                              reason=''):
        """Handle the requestGeneric reply from the GUI server

        Generic requests are supposed to have a `token` in the input arguments
        Unfolding the `token` should provide a request handler

        """
        token = request["args.token"]
        handler = self._request_handlers.pop(
            token, lambda success, reply: None)
        reply = reply if success is True else reason
        # Wrap the handler invocation in an exception swallower
        try:
            handler(success, reply)
        except Exception as ex:
            # But at least show something when exceptions are being swallowed
            msg = ('Exception of type "{}" caught in callback handler "{}" '
                   'with reply:\n{}')
            print(msg.format(type(ex).__name__, handler, reply))

    def handle_log(self, messages):
        broadcast_event(KaraboEvent.LogMessages, {'messages': messages})

    def handle_configurationFromPast(self, **info):
        success = info['success']
        # NOTE: Show a reasonable time format!
        time = info['time']
        time = Timestamp(time).toLocal()
        deviceId = info['deviceId']
        if not success:
            reason = info['reason']
            msg = ("The configuration of `{}` requested at time point `{}` "
                   "was not retrieved!".format(deviceId, time))
            messagebox.show_error(msg, details=reason)
            return
        config = info['config']
        config_time = info['configTimepoint']
        config_time = Timestamp(config_time).toLocal()
        time_match = info['configAtTimepoint']
        broadcast_event(KaraboEvent.ShowConfigurationFromPast,
                        {'deviceId': deviceId, 'configuration': config,
                         'time': time, 'config_time': config_time,
                         'time_match': time_match})

    def handle_brokerInformation(self, **info):
        self._set_server_information(info)

    def handle_serverInformation(self, **info):
        self._set_server_information(info)

    @show_wait_cursor
    def handle_systemTopology(self, systemTopology):
        self._topology.initialize(systemTopology)

        devices, servers = _extract_topology_devices(systemTopology)
        broadcast_event(KaraboEvent.SystemTopologyUpdate,
                        {'devices': devices, 'servers': servers})

        for instance_id, class_id, _ in devices:
            if class_id == 'AlarmService':
                self._request_alarm_info(instance_id)
            elif class_id == 'DaemonManager':
                broadcast_event(KaraboEvent.ShowDaemonService,
                                {'instanceId': instance_id})

    def handle_systemVersion(self, **info):
        """Handle the version number reply from the GUI server"""
        pass

    def handle_topologyUpdate(self, changes):
        devices, servers = self._topology.topology_update(changes)

        gone_instanceIds = []
        # Did an alarm system leave our topology?
        for instance_id, class_id, _ in devices:
            if class_id == 'AlarmService':
                self._alarm_model.reset_alarms(instance_id)
                broadcast_event(KaraboEvent.RemoveAlarmServices,
                                {'instanceId': instance_id})
            elif class_id == 'DaemonManager':
                broadcast_event(KaraboEvent.RemoveDaemonService,
                                {'instanceId': instance_id})
            gone_instanceIds.append(instance_id)

        # Update topology interested listeners!
        new_devices, new_servers = _extract_topology_devices(
            changes['new'])

        # Tell the GUI about various devices or servers that are alive
        for instance_id, class_id, _ in new_devices:
            if class_id == 'AlarmService':
                self._request_alarm_info(instance_id)
            elif class_id == 'ProjectManager':
                broadcast_event(KaraboEvent.ProjectDBConnect,
                                {'device': instance_id})
            elif class_id == 'DaemonManager':
                broadcast_event(KaraboEvent.ShowDaemonService,
                                {'instanceId': instance_id})

        devices.extend(new_devices)
        servers.extend(new_servers)
        broadcast_event(KaraboEvent.SystemTopologyUpdate,
                        {'devices': devices, 'servers': servers})

        broadcast_event(KaraboEvent.ClearConfigurator,
                        {'devices': gone_instanceIds})

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
        broadcast_event(KaraboEvent.SystemTopologyUpdate,
                        {'devices': devices, 'servers': servers})

        # Tell the GUI about various devices or servers that are alive
        for instance_id, class_id, _ in devices:
            if class_id == 'AlarmService':
                self._request_alarm_info(instance_id)
            elif class_id == 'ProjectManager':
                broadcast_event(KaraboEvent.ProjectDBConnect,
                                {'device': instance_id})

    def handle_instanceUpdated(self, topologyEntry):
        self._topology.instance_updated(topologyEntry)

    def handle_instanceGone(self, instanceId, instanceType):
        """Remove ``instance_id`` from topology and update"""
        # Tell the GUI about various devices that are now gone
        self._broadcast_if_of_type('AlarmService', instanceId,
                                   KaraboEvent.RemoveAlarmServices)

        # Update the system topology
        devices, servers = self._topology.instance_gone(instanceId,
                                                        instanceType)
        # Broadcast the change to listeners
        broadcast_event(KaraboEvent.SystemTopologyUpdate,
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
        broadcast_event(KaraboEvent.ClearConfigurator,
                        {'devices': [instanceId]})

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

        # Refresh the configurator iff this proxy is already showing
        broadcast_event(KaraboEvent.UpdateDeviceConfigurator,
                        {'proxy': proxy})

    def handle_deviceConfiguration(self, deviceId, configuration):
        self._topology.device_config_updated(deviceId, configuration)

    def handle_deviceConfigurations(self, configurations):
        """Update a bulk series of device configurations

        The arriving hash provides the deviceId's as keys and configurations
        as their values.
        """
        for deviceId, config in configurations.items():
            self._topology.device_config_updated(deviceId, config)

    def handle_executeReply(self, **info):
        """Handle the execute reply of the gui server"""
        success = info['success']
        if not success:
            reason = info.get('failureReason') or info.get('reason')
            input_info = info['input']
            deviceId = input_info['deviceId']
            command = input_info['command']
            text = (f'Execute slot <b>{command}</b> of device '
                    f'<b>{deviceId}</b> has encountered an error!'
                    f'<br><br>'
                    f'The reason is probably: <br>'
                    f'<i>{get_error_message(reason)}</i>'
                    f'<br><br>'
                    f'Click "Show Details..." for more information.')
            messagebox.show_error(text, details=reason)

    def handle_propertyHistory(self, **info):
        """The handler of the property history.

        The `info` dictionary contains:

            deviceId (str): the deviceId of the property
            property (str): the key of the property
            success (bool): success boolean of the request
            data (HashList): the data returned by the historic data request.

            failureReason (str): With Karabo <= 2.11, the failure reason is
                                 provided here.
            reason: Forward compatibility for future releases
        """
        success = info["success"]
        if success:
            deviceId = info["deviceId"]
            key = info["property"]
            data = info.get("data", None)
            if data is not None:
                device_proxy = self._topology.get_device(deviceId)
                device_proxy.publish_historic_data(key, data)
        else:
            # XXX: Forward compatibility!
            # The GUI server of future Karabo versions (> 2.6.X) will report
            # here about failed history request and their reason.
            pass

    def handle_runConfigSourcesInGroup(self, **info):
        # This is DEPRECATED
        pass

    # ---------------------------------------------------------------------
    # Current Configuration Interface

    def handle_listConfigurationFromName(self, success, request,
                                         reply, reason=''):
        """Handle the reply of the ListConfigurationsFromName slot"""
        deviceId = request['args.deviceId']
        if not success:
            messagebox.show_error(f"Requesting a list of configurations for "
                                  f"{deviceId} failed!", details=reason)
            return

        broadcast_event(KaraboEvent.ListConfigurationUpdated,
                        {'items': reply['items'],
                         'deviceId': deviceId})

    def handle_getConfigurationFromName(self, success, request,
                                        reply, reason=''):
        deviceId = request['args.deviceId']
        if not success:
            messagebox.show_error(f"Requesting a configuration for {deviceId} "
                                  f"failed!", details=reason)
            return

        item = reply['item']
        broadcast_event(KaraboEvent.ShowConfigurationFromName,
                        {'configuration': item['config'],
                         'name': item['name'],
                         'deviceId': deviceId})

    def handle_saveConfigurationFromName(self, success, request,
                                         reply, reason=''):
        deviceId = request['args.deviceIds'][0]
        if not success:
            messagebox.show_error(f"Saving a configuration for {deviceId} "
                                  f"failed!", details=reason)
            return

        if request.get('update', False):
            get_network().onListConfigurationFromName(deviceId)

    # ---------------------------------------------------------------------
    # Current Project Interface

    def handle_projectBeginUserSession(self, reply):
        pass

    def handle_projectEndUserSession(self, reply):
        pass

    @project_db_handler()
    def handle_projectListDomains(self, reply):
        # ``reply`` is a Hash containing a list of domain names
        broadcast_event(KaraboEvent.ProjectDomainsList,
                        {'items': reply['domains']})

    @project_db_handler()
    def handle_projectListItems(self, reply):
        # ``reply`` is a Hash containing a list of item hashes
        broadcast_event(KaraboEvent.ProjectItemsList,
                        {'items': reply.get('items', [])})

    def handle_projectListProjectManagers(self, reply):
        # ``reply`` is a list of strings
        broadcast_event(KaraboEvent.ProjectManagersList,
                        {'items': reply})

    @project_db_handler(fall_through=True)
    def handle_projectLoadItems(self, reply):
        # ``reply`` is a Hash containing a list of item hashes
        success = reply.get('success', True)
        d = {'success': success, 'items': reply.get('items', [])}
        broadcast_event(KaraboEvent.ProjectItemsLoaded, d)

    @project_db_handler(fall_through=True)
    def handle_projectSaveItems(self, reply):
        # ``reply`` is a Hash containing a list of item hashes
        success = reply.get('success', True)
        d = {'success': success, 'items': reply.get('items', [])}
        broadcast_event(KaraboEvent.ProjectItemsSaved, d)

    @project_db_handler()
    def handle_projectUpdateAttribute(self, reply):
        # ``reply`` is a Hash containing a list of item hashes
        broadcast_event(KaraboEvent.ProjectAttributeUpdated,
                        {'items': reply['items']})

    def handle_projectUpdate(self, **info):
        """Handle the project update signal from the project manager

        The information is forwarded by the gui server to notify clients about
        updated project items.
        """
        projects = info['info']
        # Get is only required when we connect to an old ecosystem, which will
        # always provide this update msg without client information!
        client_id = projects.get('client', KARABO_CLIENT_ID)
        if client_id == KARABO_CLIENT_ID:
            # If we have been the client saving the project, we are not
            # interested any further!
            return
        uuids = projects['projects']
        broadcast_event(KaraboEvent.ProjectUpdated, {'uuids': uuids})

    # ---------------------------------------------------------------------

    def handle_notification(self, **info):
        """Handle notification events from the GUI server
        """
        message = info.get('message', 'Notification from GUI Server is empty!')
        messagebox.show_warning(message)

    def handle_networkData(self, name, data, meta=None):
        """This method handles the big data chunks coming from Karabo
        pipelines to `GuiServerDevice`. To keep the GUI responsive the
        displaying of this data is delayed here.
        """

        def show_data():
            if name not in self._big_data:
                return
            data_hash, meta_hash = self._big_data.pop(name)
            device_id, prop_path = name.split(":")
            device_proxy = self._topology.get_device(device_id)
            binding = device_proxy.get_property_binding(prop_path)
            timestamp = Timestamp.fromHashAttributes(
                meta_hash['timestamp', ...])
            if binding is not None:
                # Note: Binding can be `None` on startup when schema was not
                # arriving and the application closing...
                apply_fast_data(data_hash, binding.value.schema, timestamp)
                device_proxy.config_update = True
                if self._show_big_data_proc:
                    proc = Timestamp().toTimestamp() - timestamp.toTimestamp()
                    info = {'name': name, 'proc': proc}
                    broadcast_event(KaraboEvent.BigDataProcessing, info)
            # Let the GUI server know we have processed this chunk
            get_network().onRequestNetwork(name)

        self._big_data[name] = (data, meta)
        executeLater(show_data, Priority.BIG_DATA)

    def handle_initReply(self, deviceId, success, message):
        device = self._topology.get_device(deviceId)
        if device is not None:
            data = {'device': device, 'success': success, 'message': message}
            # Create KaraboBroadcastEvent
            broadcast_event(KaraboEvent.DeviceInitReply, data)
            if not success:
                text = ("The instance <b>{}</b> could not be "
                        "instantiated.".format(deviceId))
                messagebox.show_error(text, details=message)

    def handle_alarmInit(self, instanceId, rows):
        """Show initial update for ``AlarmService`` with given ``instanceId``
           and all the information given in the ``Hash`` ``rows``.
        """
        if not rows.empty():
            data = extract_alarms_data(instanceId, rows)
            self._topology.update_alarms_info(data)
            self._alarm_model.init_alarms_info(data)

    def handle_alarmUpdate(self, instanceId, rows):
        """Show update for ``AlarmService`` with given ``instanceId`` and all
           the information given in the ``Hash`` ``rows``.
        """
        if not rows.empty():
            data = extract_alarms_data(instanceId, rows)
            self._topology.update_alarms_info(data)
            self._alarm_model.update_alarms_info(data)

    # ------------------------------------------------------------------
    # Private methods

    def _request_alarm_info(self, instance_id):
        """Handle the arrival of the `AlarmService` device"""
        get_network().onRequestAlarms(instance_id)

    def _broadcast_if_of_type(self, class_id, instance_id, event_type):
        """If a device is of a particular type `class_id`, broadcast an event.
        """
        attrs = self._topology.get_attributes('device.' + instance_id)
        if attrs is not None:
            if class_id == attrs.get('classId', ''):
                broadcast_event(event_type, {'instanceId': instance_id})
                return True
        return False

    def _set_server_information(self, info):
        read_only = info.get('readOnly', False)
        get_network().set_server_information(read_only=read_only)
        broadcast_event(KaraboEvent.BrokerInformationUpdate, info)


# ------------------------------------------------------------------


def _extract_topology_devices(topo_hash):
    """Get all the devices and their classes out of a system topology update.
    """
    devices, servers = [], []

    if 'device' in topo_hash:
        for device_id, _, attrs in topo_hash['device'].iterall():
            class_id = attrs.get('classId', 'unknown-class')
            status = attrs.get('status', 'ok')
            devices.append((device_id, class_id, ProxyStatus(status)))

    if 'macro' in topo_hash:
        for device_id, _, attrs in topo_hash['macro'].iterall():
            class_id = attrs.get('classId', 'unknown-class')
            devices.append((device_id, class_id, ProxyStatus.OK))

    if 'server' in topo_hash:
        for server_id, _, attrs in topo_hash['server'].iterall():
            host = attrs.get('host', 'UNKNOWN')
            status = attrs.get('status', 'ok')
            servers.append((server_id, host, ProxyStatus(status)))

    return devices, servers
