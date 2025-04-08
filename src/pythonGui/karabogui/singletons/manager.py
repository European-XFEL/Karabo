#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
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
#############################################################################
import logging
import uuid
from functools import wraps
from inspect import signature
from weakref import WeakKeyDictionary, WeakValueDictionary

from qtpy.QtCore import QObject, Slot
from qtpy.QtWidgets import QMessageBox

import karabogui.access as krb_access
from karabo.native import AccessLevel, AccessMode, Assignment, Hash, Timestamp
from karabogui import messagebox
from karabogui.background import Priority, executeLater
from karabogui.binding.api import (
    ProxyStatus, apply_fast_data, extract_configuration)
from karabogui.const import KARABO_CLIENT_ID
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.logger import get_logger
from karabogui.singletons.api import get_config, get_network, get_topology
from karabogui.util import get_reason_parts, move_to_cursor, show_wait_cursor

from .util import realign_topo_hash


def project_db_handler(fall_through=False):
    """Decorator for project DB handlers to cut down on boilerplate"""

    def inner(handler):
        @wraps(handler)
        def wrapped(self, success, request, reply, reason=""):
            if not success:
                error, details = get_reason_parts(reason)
                messagebox.show_error(error, details=details)
            elif not reply.get("success", True):
                success = False
                reason = reply.get("reason")
                error, details = get_reason_parts(reason)
                messagebox.show_error(error, details=details)
            if fall_through or success:
                return handler(self, success, request, reply, reason=reason)

        return wrapped

    return inner


def getLevelName(level):
    """Convert a logging level from the gui server to an integer"""
    return logging.getLevelName(level.upper())


class Manager(QObject):
    def __init__(self, parent=None):
        super().__init__(parent=parent)
        # this dictionary maps the deviceIds to a weak reference of the
        # DeviceProxy instances waiting for a reply from the server
        self._waiting_devices = WeakValueDictionary()
        # this dictionary maps a weak reference DeviceProxy with the list of
        # property proxies waiting to be heard
        self._waiting_properties = WeakKeyDictionary()
        # The system topology singleton
        self._topology = get_topology()
        # A dict which includes big networkData
        self._big_data = {}
        # Handler callbacks for `handle_requestGeneric`
        self._request_handlers = {}

        # Awaiting device instantiations
        self._awaiting_instantiations = set()
        self._show_big_data_proc = False
        network = get_network()
        network.signalServerConnectionChanged.connect(
            self.onServerConnectionChanged)
        network.signalReceivedData.connect(self.onReceivedData)

        if get_config()["development"]:
            self.toggleBigDataPerformanceMonitor()

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
        """Instantiate a project device"""
        proxy = self._topology.get_project_device_proxy(
            deviceId, serverId, classId)
        if proxy is None:
            # This should never happen!
            return

        def _instantiate_device():
            """Instantiate a project device"""
            nonlocal config

            if proxy.online:
                # Note: Device was instantiated from a different place
                # and not us, conflict! This can happen especially from an
                # awaiting instantiation!
                messagebox.show_error(
                    f"Aborting instantiation for device {deviceId} because it "
                    "is already online!")
                return

            if config is None:
                config = extract_configuration(proxy.binding)

            schema = self._topology.get_schema(serverId, classId)
            if schema is None:
                # XXX: This should not happen anymore. However, we go safe here
                messagebox.show_error(
                    f"Aborting instantiation for device {deviceId} due to "
                    "a missing schema!")
                return

            # Remove all obsolete paths which are not anymore in the schema
            # Note: This is supposed to throw on the device server level
            obsolete_paths = [pth for pth, _, _ in Hash.flat_iterall(config)
                              if pth not in schema.hash]
            for key in obsolete_paths:
                config.erase(key)

            # Remove all read only and assignment internal paths
            readonly_paths = [pth for pth, _, _ in Hash.flat_iterall(config)
                              if schema.hash[pth, "accessMode"] ==
                              AccessMode.READONLY.value or
                              schema.hash[pth, "assignment"] ==
                              Assignment.INTERNAL.value]
            for key in readonly_paths:
                config.erase(key)

            # Send signal to network
            get_network().onInitDevice(serverId, classId, deviceId, config)

        if len(proxy.binding.value) > 0:
            _instantiate_device()
        else:
            # We don't have a schema and have to defer the instantiation.
            # We are sneaky and wait until the offline configuration has
            # been applied!
            get_logger().info("Requesting class schema with classId "
                              f"<b>{classId}</b> for <b>{deviceId}</b> "
                              "and deferring instantiation")

            def _config_update_handler():
                proxy.on_trait_change(_config_update_handler, "config_update",
                                      remove=True)
                if deviceId in self._awaiting_instantiations:
                    self._awaiting_instantiations.discard(deviceId)
                    _instantiate_device()

            self._awaiting_instantiations.add(deviceId)
            proxy.on_trait_change(_config_update_handler, "config_update")
            proxy.ensure_class_schema()

    def callDeviceSlot(self, handler, instance_id, slot_name, params):
        """Call a device slot using the `requestGeneric` mechanism.

        See karabogui.request for more details.
        """
        # Generate a unique token for the transaction
        token = uuid.uuid4().hex
        assert token not in self._request_handlers

        # Set the callback handler
        assert callable(handler)
        self._request_handlers[token] = handler
        # Call the GUI server
        get_network().onExecuteGeneric(instance_id, slot_name, params,
                                       token=token)
        return token

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

        # We can also shutdown a device with `Shutdown all devices` while
        # waiting for their instantiation. We make sure to delete our request
        # here!
        self._awaiting_instantiations.discard(deviceId)

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
        # Note: We shutdown a server although we want to instantiate devices
        # from there.
        if self._awaiting_instantiations:
            deviceIds = {deviceId for deviceId, project_device
                         in self._topology._project_devices.items()
                         if project_device.server_id == serverId}
            discard = deviceIds.intersection(self._awaiting_instantiations)
            for deviceId in discard:
                self._awaiting_instantiations.discard(deviceId)
        get_network().onKillServer(serverId)

    @Slot(object)
    def onReceivedData(self, hsh):
        handler = getattr(self, "handle_" + hsh["type"])
        kwargs = {k: v for k, v in hsh.items() if k != "type"}
        handler(**kwargs)

    @Slot(bool)
    def onServerConnectionChanged(self, isConnected):
        """If the server connection is changed, the model needs an update.
        """
        # Broadcast event to all panels which need a reset
        broadcast_event(KaraboEvent.NetworkConnectStatus,
                        {'status': isConnected})

        # Clear the system topology
        if not isConnected:
            self._topology.clear()

            # Delete any request of instantiating devices
            self._awaiting_instantiations.clear()

            # Any pending requests are effectively timed-out and removed
            pending = len(self._request_handlers)
            if pending:
                get_logger().error(
                    f"Erased <b>{pending}</b> pending generic requests due to "
                    "client disconnect.")
            self._request_handlers.clear()

    def handle_setLogPriorityReply(self, **info):
        """Handle the log priority reconfiguration reply of the server"""
        if not info.get("success", True):
            reason = info.get("reason")
            error, details = get_reason_parts(reason)
            input_info = info["input"]
            instanceId = input_info["instanceId"]
            priority = input_info["priority"]
            log_text = (f"Log level reconfiguration of <b>{instanceId}</b>"
                        f" with priority <b>{priority}</b> failed.<br>"
                        f"The reason is:<br><i>{error}</i>")
            get_logger().error(log_text)
            messagebox.show_error(log_text, details=details)

    def handle_reconfigureReply(self, **info):
        """Handle the reconfigure reply of the gui server"""
        success = info['success']
        input_info = info['input']
        deviceId = input_info['deviceId']
        if not success:
            reason = info.get('failureReason') or info.get('reason')
            reason, details = get_reason_parts(reason)
            # clear the waiting queue
            device = self._waiting_devices.pop(deviceId, None)
            if device is None:
                return
            props = self._waiting_properties.pop(device, [])
            paths = set(input_info['configuration'].paths())
            pending = [property_proxy for property_proxy in props
                       if property_proxy.path not in paths]
            if pending:
                # in case multiple updates are sent and only few failed
                self.expect_properties(device, pending)
            text = (f'Device reconfiguration of <b>{deviceId}</b> encountered '
                    'an error. <br><br>The reason is:<br>'
                    f'<i>{reason}</i><br>')
            if details is not None:
                text += '<br>Click "Show Details..." for more information.'
            # Provide a text for the logger!
            log_text = (f"Device reconfiguration of <b>{deviceId}</b> with "
                        f"properties <b>{', '.join(paths)}</b> failed")
            get_logger().error(log_text)
            messagebox.show_error_at_cursor(text, details=details)
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
                # Erase the successful configured edit_value and wait for the
                # config update
                prop_proxy.trait_setq(edit_value=None)
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
        token = request["token"]
        handler = self._request_handlers.pop(token, None)
        if handler is None:
            return
        reply = reply if success is True else reason
        # Wrap the handler invocation in an exception swallower
        try:
            sig = signature(handler)
            if len(sig.parameters) == 2:
                handler(success, reply)
            else:
                handler(success, reply, request)
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
            reason, details = get_reason_parts(reason)
            msg = (f"The configuration of `{deviceId}` requested at time point"
                   f" `{time}` was not retrieved!<br><br>"
                   "The reason is:<br>"
                   f"<i>{reason}</i>")
            messagebox.show_error(msg, details=details)
            return

        config = info['config']
        config_time = info['configTimepoint']
        config_time = Timestamp(config_time).toLocal()
        time_match = info['configAtTimepoint']

        # We might deal with old gui server < 2.11
        preview = info.get('preview', False)
        broadcast_event(KaraboEvent.ShowConfigurationFromPast,
                        {'deviceId': deviceId, 'configuration': config,
                         'time': time, 'config_time': config_time,
                         'preview': preview, 'time_match': time_match})

    def handle_brokerInformation(self, **info):
        self._set_server_information(info)

    def handle_serverInformation(self, **info):
        self._set_server_information(info)

    def handle_loginInformation(self, **info):
        """Handle the login information from the gui server

        If the gui server is in readOnly mode, the readOnly boolean is included
        here as well since 2.20.X.
        """
        get_config()['username'] = info["username"]
        read_only = info.get("readOnly", False)
        if read_only:
            access = AccessLevel.OBSERVER
        else:
            access = AccessLevel(info["accessLevel"])

        krb_access.HIGHEST_ACCESS_LEVEL = access
        if krb_access.GLOBAL_ACCESS_LEVEL != access:
            krb_access.GLOBAL_ACCESS_LEVEL = access
            broadcast_event(KaraboEvent.AccessLevelChanged, {})

        broadcast_event(KaraboEvent.LoginUserChanged, {})

    def handle_onBeginTemporarySession(self, **info):
        """Handle the response from gui server on starting the temporary
        session request by the user"""
        if info["success"]:
            krb_access.TEMPORARY_SESSION_USER = info.get("username")
            krb_access.TEMPORARY_SESSION_WARNING = False
            access_level = AccessLevel(info["accessLevel"])

            krb_access.HIGHEST_ACCESS_LEVEL = access_level
            if krb_access.GLOBAL_ACCESS_LEVEL != access_level:
                krb_access.GLOBAL_ACCESS_LEVEL = access_level
                broadcast_event(KaraboEvent.AccessLevelChanged, {})

            broadcast_event(KaraboEvent.LoginUserChanged, {})
            broadcast_event(KaraboEvent.TemporarySession, {})

    def handle_onEndTemporarySession(self, **info):
        """Handle the response from gui server on end temporary session
        request by user"""
        self._end_temporary_session(info)

    def handle_onTemporarySessionExpired(self, **info):
        """Handle the temporary session expired message from gui server """
        self._end_temporary_session(info)

    def _end_temporary_session(self, info):
        level_before = info.get("levelBeforeTemporarySession")
        if level_before is None:
            return
        access_level = AccessLevel(level_before)
        if krb_access.HIGHEST_ACCESS_LEVEL != access_level:
            krb_access.HIGHEST_ACCESS_LEVEL = access_level

        if krb_access.GLOBAL_ACCESS_LEVEL != access_level:
            krb_access.GLOBAL_ACCESS_LEVEL = access_level
            broadcast_event(KaraboEvent.AccessLevelChanged, {})

        krb_access.TEMPORARY_SESSION_USER = None
        broadcast_event(KaraboEvent.LoginUserChanged, {})
        broadcast_event(KaraboEvent.TemporarySession, {})

    def handle_onEndTemporarySessionNotice(self, **info):
        """Broadcast before the temporary session ends"""
        krb_access.TEMPORARY_SESSION_WARNING = True
        broadcast_event(KaraboEvent.TemporarySession, {})

    @show_wait_cursor
    def handle_systemTopology(self, systemTopology):
        systemTopology["server"] = realign_topo_hash(
            systemTopology["server"], "host")
        self._topology.initialize(systemTopology)

        devices, servers = _extract_topology_devices(systemTopology)
        broadcast_event(KaraboEvent.SystemTopologyUpdate,
                        {'devices': devices, 'servers': servers})

        for instance_id, class_id, _ in devices:
            if class_id == 'DaemonManager':
                broadcast_event(KaraboEvent.ShowDaemonService,
                                {'instanceId': instance_id})

    def handle_systemVersion(self, **info):
        """Handle the version number reply from the GUI server"""
        pass

    def handle_topologyUpdate(self, changes):
        devices, servers = self._topology.topology_update(changes)

        gone_instanceIds = []
        for instance_id, class_id, _ in devices:
            if class_id == 'DaemonManager':
                broadcast_event(KaraboEvent.RemoveDaemonService,
                                {'instanceId': instance_id})
            gone_instanceIds.append(instance_id)

        # Update topology interested listeners!
        new_devices, new_servers = _extract_topology_devices(
            changes['new'])

        # Tell the GUI about various devices or servers that are alive
        for instance_id, class_id, _ in new_devices:
            if class_id == 'ProjectManager':
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
        pass

    def handle_instanceUpdated(self, topologyEntry):
        pass

    def handle_instanceGone(self, instanceId, instanceType):
        pass

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
            reason, details = get_reason_parts(reason)
            input_info = info['input']
            deviceId = input_info['deviceId']
            command = input_info['command']
            text = (f'Execute slot <b>{command}</b> of device '
                    f'<b>{deviceId}</b> has encountered an error!'
                    '<br><br>'
                    'The reason is:<br>'
                    f'<i>{reason}</i>'
                    '<br>')
            if details is not None:
                text += '<br>Click "Show Details..." for more information.'
            messagebox.show_error(text, details=details)

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
            # Provide a logger message on failure for developers
            deviceId = info["deviceId"]
            key = info["property"]
            text = (f"Historic data request for property <b>{key}</b> "
                    f"of device <b>{deviceId}</b> failed.")
            get_logger().debug(text)

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
                                  "failed!", details=reason)
            return

        item = reply['item']
        broadcast_event(KaraboEvent.ShowConfigurationFromName,
                        {'configuration': item['config'],
                         'preview': request['preview'],
                         'name': item['name'],
                         'deviceId': deviceId})

    def handle_deleteConfigurationFromName(self, success, request,
                                           reply, reason=""):
        deviceId = request['args.deviceId']
        name = request['args.name']
        if not success:
            messagebox.show_error(f"Delete a configuration for {deviceId} "
                                  f"with name {name} failed!", details=reason)
            return

        get_network().onListConfigurationFromName(deviceId)

    def handle_saveConfigurationFromName(self, success, request,
                                         reply, reason=''):
        deviceId = request['args.deviceIds'][0]
        if not success:
            messagebox.show_error(f"Saving a configuration for {deviceId} "
                                  "failed!", details=reason)
            return

        if request.get('update', False):
            get_network().onListConfigurationFromName(deviceId)

    # ---------------------------------------------------------------------
    # Current Project Interface

    def handle_projectBeginUserSession(self, **reply):
        pass

    def handle_projectEndUserSession(self, **reply):
        pass

    @project_db_handler()
    def handle_projectListDomains(self, success, request, reply, reason=""):
        # ``reply`` is a Hash containing a list of domain names
        broadcast_event(KaraboEvent.ProjectDomainsList,
                        {'items': reply.get('domains', [])})

    @project_db_handler()
    def handle_projectListItems(self, success, request, reply, reason=""):
        broadcast_event(KaraboEvent.ProjectItemsList,
                        {'items': reply.get('items', [])})

    def handle_projectListProjectsWithDevice(self, success, request,
                                             reply, reason=""):
        error_details = None
        if not success:
            # An error occurred at the GUI Server level
            # (or exception in Project Manager)
            error_details = reason
        elif not reply['success']:
            # An error occurred at the Project Manager level
            error_details = reply['reason']
        if error_details is not None:
            broadcast_event(
                KaraboEvent.ProjectFindWithDevice,
                {'projects': [], 'error': error_details})
        else:
            projects = reply.get('projects', [])
            broadcast_event(
                KaraboEvent.ProjectFindWithDevice, {
                    'projects': projects, 'error': None})

    def handle_projectListProjectManagers(self, reply):
        pass

    @project_db_handler(fall_through=True)
    def handle_projectLoadItems(self, success, request, reply, reason=""):
        d = {'success': success, 'items': reply.get('items', [])}
        broadcast_event(KaraboEvent.ProjectItemsLoaded, d)

    @project_db_handler(fall_through=True)
    def handle_projectSaveItems(self, success, request, reply, reason=""):
        data = {'success': success, 'items': reply.get('items', [])}
        broadcast_event(KaraboEvent.ProjectItemsSaved, data)

    @project_db_handler()
    def handle_projectUpdateAttribute(self, success, request, reply,
                                      reason=""):
        data = {'items': reply['items']}
        broadcast_event(KaraboEvent.ProjectAttributeUpdated, data)

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
        message = info.get("message", "")
        content_type = info.get("contentType", "")
        if content_type == "banner":
            broadcast_event(KaraboEvent.ServerNotification, info)
        elif content_type == "logger":
            text = info.get("message", "")
            level = getLevelName(info.get("level", "INFO"))
            get_logger().log(level, text)
        elif message:
            messagebox.show_warning(message)

    def handle_networkData(self, name, data, meta=None):
        """This method handles the big data chunks coming from Karabo
        pipelines to `GuiServerDevice`. To keep the GUI responsive the
        displaying of this data is delayed here.
        """

        def show_data():
            item = self._big_data.pop(name, None)
            if item is None:
                # XXX: This should never happen, but keep going
                get_network().onError(
                    f"Received big data although not scheduled for {name}.")
                get_network().onRequestNetwork(name)
                return
            data_hash, meta_hash = item
            device_id, prop_path = name.split(":")
            device_proxy = self._topology.get_device(device_id)
            if not device_proxy.online:
                return
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
                reason, details = get_reason_parts(message)
                text = (f'The instance <b>{deviceId}</b> could not be '
                        'instantiated.. <br><br>The reason is:<br>'
                        f'<i>{reason}</i><br>')
                if details is not None:
                    text += '<br>Click "Show Details..." for more information.'
                messagebox.show_error(text, details=details)

    def handle_alarmInit(self, instanceId, rows):
        """Show initial update for ``AlarmService`` with given ``instanceId``
           and all the information given in the ``Hash`` ``rows``.
        """

    def handle_alarmUpdate(self, instanceId, rows):
        """Show update for ``AlarmService`` with given ``instanceId`` and all
           the information given in the ``Hash`` ``rows``.
        """

    def handle_listDestinations(self, success, request, reply, reason=""):
        """
        Broadcast the available proposals in the Topic, fetched by the
        KaraboLogBook, so that the PreviewDialog can  list them.
        """
        destinations = reply.get("destinations")
        broadcast_event(KaraboEvent.ActiveDestinations, destinations)

    def handle_saveLogBook(self, success, request, reply, reason=""):
        """
        Show messagebox when the KaraboLogBook fails to send the message to
        the logbook.
        """
        if success:
            args = request.get("args")
            data_type = args.get("dataType")
            message = f"Posted the {data_type} to LogBook successfully"
            get_logger().info(message)
        else:
            error, details = get_reason_parts(reason)
            messagebox.show_warning(title="Send to LogBook Failed",
                                    text=error, details=details)

    # ------------------------------------------------------------------
    # Private methods

    def _set_server_information(self, info):
        read_only = info.get('readOnly', False)
        get_network().set_server_information(read_only=read_only)
        broadcast_event(KaraboEvent.ServerInformationUpdate, info)

    # ------------------------------------------------------------------


def _extract_topology_devices(topo_hash):
    """Get all the devices and their classes out of a system topology update.
    """
    devices, servers = [], []

    if 'device' in topo_hash:
        for device_id, _, attrs in topo_hash['device'].iterall():
            # XXX: remove this check after #122877 is fixed on the GUI server
            if not attrs:
                msg = ("skipping malformed topology entry "
                       f"for <b>{device_id}</b>")
                get_logger().error(msg)
                continue
            class_id = attrs['classId']
            devices.append((device_id, class_id, ProxyStatus.ONLINE))

    if 'macro' in topo_hash:
        for device_id, _, attrs in topo_hash['macro'].iterall():
            # XXX: remove this check after #122877 is fixed on the GUI server
            if not attrs:
                msg = ("skipping malformed topology entry "
                       f"for <b>{device_id}</b>")
                get_logger().error(msg)
                continue
            class_id = attrs['classId']
            devices.append((device_id, class_id, ProxyStatus.ONLINE))

    if 'server' in topo_hash:
        for server_id, _, attrs in topo_hash['server'].iterall():
            host = attrs['host']
            servers.append((server_id, host, ProxyStatus.ONLINE))

    return devices, servers
