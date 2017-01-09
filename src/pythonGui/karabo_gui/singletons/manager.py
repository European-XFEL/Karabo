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
from karabo.middlelayer import Hash, Schema, XMLWriter, XMLParser
from karabo_gui.configuration import BulkNotifications
from karabo_gui.events import (
    broadcast_event, KaraboBroadcastEvent, KaraboEventSender)
from karabo_gui.messagebox import MessageBox
from karabo_gui.navigationtreemodel import NavigationTreeModel
from karabo_gui.singletons.api import get_network, get_project_model
from karabo_gui.topology import getClass
from karabo_gui.util import (
    getOpenFileName, getSaveFileName, getSchemaAttributeUpdates)


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
        data['is_changing'] = False
        broadcast(KaraboBroadcastEvent(KaraboEventSender.DeviceStateChanged,
                                       data))


class Manager(QObject):
    def __init__(self, parent=None):
        super(Manager, self).__init__(parent=parent)

        # Model for navigation views
        self.systemTopology = NavigationTreeModel(self)
        self.systemTopology.selectionModel.selectionChanged.connect(
            self.onNavigationTreeModelSelectionChanged)

        network = get_network()
        network.signalServerConnectionChanged.connect(
            self.onServerConnectionChanged)
        network.signalReceivedData.connect(self.onReceivedData)

        # Sets all parameters to start configuration
        self.reset()

    # Sets all parameters to start configuration
    def reset(self):
        self.systemHash = None

        # Map stores { (serverId, class), Configuration }
        self.serverClassData = {}
        # Map stores { (serverId, class), Schema }
        self._immutableServerClassData = {}
        # Map stores { deviceId, Configuration }
        self.deviceData = {}

    def _clearDeviceParameterPage(self, deviceId):
        conf = self.deviceData.get(deviceId)
        if conf is None:
            return
        # Clear corresponding parameter page
        if conf.parameterEditor is not None:
            conf.parameterEditor.clear()

        if conf.descriptor is not None:
            conf.redummy()

    def _clearServerClassParameterPages(self, serverClassIds):
        for serverClassId in serverClassIds:
            conf = self.serverClassData.get(serverClassId)
            if conf is None:
                continue

            # Clear corresponding parameter page
            if conf.parameterEditor is not None:
                conf.parameterEditor.clear()

            if conf.descriptor is not None:
                conf.redummy()

    def _extractAlarmServices(self):
        """ This method extracts the existing devices of type ``AlarmService``
            of the ``self.systemHash`` and returns the instance ids in a list.
        """
        instanceIds = []
        if 'device' in self.systemHash:
            for deviceId, _, attrs in self.systemHash['device'].iterall():
                classId = attrs.get("classId", "unknown-class")
                if classId == 'AlarmService':
                    instanceIds.append(deviceId)
        return instanceIds

    def initDevice(self, serverId, classId, deviceId, config=None):
        # Use standard configuration for server/classId
        conf = getClass(serverId, classId)
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
        baseSchema = self._immutableServerClassData[serverId, classId]
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

        if not isConnected:
            # Reset manager settings
            self.reset()

    def onNavigationTreeModelSelectionChanged(self, selected, deselect):
        """This slot is called whenever something of the navigation panel is
        selected. If an item was selected, the selection of the project panel
        is cleared.
        """
        if not selected.indexes():
            return

        get_project_model().q_selection_model.clear()

    def onShowConfiguration(self, conf):
        # Notify listeners
        data = {'configuration': conf}
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.ShowConfiguration, data))

    def _currentConfigurationAndClassId(self):
        """This function returns the configuration of the currently selected
        device which can be part of the systemTopology.

        Returns None, If no device is selected.
        """
        if self.systemTopology.currentIndex().isValid():
            index = self.systemTopology.currentIndex()
            indexInfo = self.systemTopology.indexInfo(index)
            deviceId = indexInfo.get("deviceId")
            classId = indexInfo.get("classId")
            serverId = indexInfo.get("serverId")

            if deviceId is not None and deviceId:
                return self.deviceData[deviceId], classId
            elif serverId is not None:
                return self.serverClassData[serverId, classId], classId
            else:
                return None, None
        else:
            return None, None

    @staticmethod
    def _loadClassConfiguration(classConfig, configHash, classId):
        if classId not in configHash:
            MessageBox.showError("Configuration load failed")
            return
        classConfig.fromHash(configHash[classId])

    def onOpenFromFile(self):
        filename = getOpenFileName(caption="Open configuration",
                                   filter="XML (*.xml)")
        if not filename:
            return

        conf, classId = self._currentConfigurationAndClassId()
        if conf is None:
            MessageBox.showError("Configuration load failed")
            return

        r = XMLParser()
        with open(filename, 'rb') as file:
            config = r.read(file.read())

        self._loadClassConfiguration(conf, config, classId)

    def onSaveToFile(self):
        """This function saves the current configuration of a device to a file.
        """
        filename = getSaveFileName(caption="Save configuration as",
                                   filter="Configuration (*.xml)",
                                   suffix="xml")
        if not filename:
            return

        conf, classId = self._currentConfigurationAndClassId()
        if conf is None:
            MessageBox.showError("Configuration save failed")
            return

        hsh, attrs = conf.toHash()
        config = Hash(classId, hsh)
        config[classId, ...] = attrs

        # Save configuration to file
        w = XMLWriter()
        with open(filename, 'wb') as file:
            w.writeToFile(config, file)

    def _deviceDataReceived(self):
        """Notify all listeners that some (class, schema, or config) data was
        received.
        """
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.DeviceDataReceived, {}))

    def handle_log(self, messages):
        data = {'messages': messages}
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.LogMessages, data))

    def handle_brokerInformation(self, **kwargs):
        get_network()._handleBrokerInformation(**kwargs)

    def handle_systemTopology(self, systemTopology):
        if self.systemHash is None:
            self.systemHash = systemTopology
        else:
            self.systemHash.merge(systemTopology, "merge")
        # Update navigation and project treemodel
        self.systemTopology.updateData(systemTopology)
        for v in self.deviceData.values():
            v.updateStatus()
        # Distribute current alarm service devices
        instanceIds = self._extractAlarmServices()
        if instanceIds:
            data = {'instanceIds': instanceIds}
            # Create KaraboBroadcastEvent
            broadcast_event(KaraboBroadcastEvent(
                KaraboEventSender.ShowAlarmServices, data))

        instanceIds = self._extractRunConfigurators()
        if instanceIds:
            data = {'instanceIds': instanceIds}
            # Create KaraboBroadcastEvent
            broadcast_event(KaraboBroadcastEvent(
                KaraboEventSender.AddRunConfigurator, data))

        # Tell the world about new devices
        devices, servers = _extract_topology_devices(systemTopology)
        data = {'devices': devices, 'servers': servers}
        broadcast_event(KaraboBroadcastEvent(
                KaraboEventSender.SystemTopologyUpdate, data))

    def handle_systemVersion(self, version):
        """ Handle the version number reply from the GUI server.
        """
        pass

    def handle_instanceNew(self, topologyEntry):
        """ This function gets the configuration for a new instance.

        If the instance already exists in the central hash, it is first
        removed from there. """
        # Check for existing stuff and remove
        detect_instances = self.systemTopology.detectExistingInstances
        instanceIds, serverClassIds = detect_instances(topologyEntry)

        log_messages = []
        for inst_id in instanceIds:
            message = {
                'timestamp': datetime.now().isoformat(),
                'type': 'INFO',
                'category': inst_id,
                'message': ('Detected dirty shutdown for instance "{}", '
                            'which is coming up now.').format(inst_id)
            }
            log_messages.append(message)

            # Clear deviceId parameter page, if existent
            self._clearDeviceParameterPage(inst_id)

        self.handle_log(log_messages)
        self._clearServerClassParameterPages(serverClassIds)

        # Update system topology with new configuration
        self.handle_instanceUpdated(topologyEntry)
        # Distribute new alarm service devices
        instanceIds = self._extractAlarmServices()
        if instanceIds:
            data = {'instanceIds': instanceIds}
            # Create KaraboBroadcastEvent
            broadcast_event(KaraboBroadcastEvent(
                KaraboEventSender.ShowAlarmServices, data))

    def handle_instanceUpdated(self, topologyEntry):
        self.handle_systemTopology(topologyEntry)
        # Topology changed so send new class schema requests
        if "server" in topologyEntry:
            # Request schema for already viewed classes, if a server is new
            for k in self.serverClassData:
                getClass(k[0], k[1])

    def handle_instanceGone(self, instanceId, instanceType):
        """ Remove instanceId from central hash and update
        """
        removed_devices, removed_servers = [], []

        if instanceType in ("device", "macro"):
            # Distribute gone alarm service devices
            instanceIds = self._extractAlarmServices()
            if instanceId in instanceIds:
                data = {'instanceIds': [instanceId]}
                # Create KaraboBroadcastEvent
                broadcast_event(KaraboBroadcastEvent(
                    KaraboEventSender.RemoveAlarmServices, data))

            instanceIds = self._extractRunConfigurators()
            if instanceId in instanceIds:
                data = {'instanceIds': [instanceId]}
                # Create KaraboBroadcastEvent
                broadcast_event(KaraboBroadcastEvent(
                    KaraboEventSender.RemoveRunConfigurator, data))

            device = self.deviceData.get(instanceId)
            if device is not None:
                device.status = "offline"
                if device.descriptor is not None:
                    device.redummy()
                # Clear corresponding parameter page
                if device.parameterEditor is not None:
                    device.parameterEditor.clear()

            # Update system topology
            self.systemTopology.eraseDevice(instanceId)

            # Record departing devices
            path = instanceType + "." + instanceId
            attributes = self.systemHash.getAttributes(path)
            classId = attributes.get("classId", "unknown-class")
            removed_devices.append((instanceId, classId, 'offline'))

            # Remove device from systemHash
            if self.systemHash is not None and path in self.systemHash:
                del self.systemHash[path]
        elif instanceType == "server":
            # Update system topology
            serverClassIds = self.systemTopology.eraseServer(instanceId)
            self._clearServerClassParameterPages(serverClassIds)

            # Record departing servers
            path = "server." + instanceId
            attributes = self.systemHash.getAttributes(path)
            host = attributes.get('host', 'UNKNOWN')
            removed_servers.append((instanceId, host, 'offline'))

            # Remove server from systemHash
            if self.systemHash is not None and path in self.systemHash:
                del self.systemHash[path]

            for v in self.deviceData.values():
                v.updateStatus()

            # Clear corresponding parameter pages
            # TODO: We need to find all the Configuration objects in the open
            # projects and clear their parameter editor objects if they happen
            # to belong to the server that was just removed!
            # This was formerly handled by the old ProjectModel class

        # Broadcast the change to listeners
        data = {'devices': removed_devices, 'servers': removed_servers}
        broadcast_event(KaraboBroadcastEvent(
                KaraboEventSender.SystemTopologyUpdate, data))

    def handle_attributesUpdated(self, reply):
        instanceId = reply["instanceId"]
        schema = reply["updatedSchema"]
        self.handle_deviceSchema(instanceId, schema)

    def handle_classSchema(self, serverId, classId, schema):
        key = (serverId, classId)
        if key not in self.serverClassData:
            print('Unrequested schema for classId {} arrived'.format(classId))
            return

        # Save a clean copy
        schemaCopy = Schema()
        schemaCopy.copy(schema)
        self._immutableServerClassData[key] = schemaCopy

        conf = self.serverClassData[key]
        if conf.descriptor is not None:
            return

        if len(schema.hash) > 0:
            # Set schema only, if data is available
            conf.setSchema(schema)
            # Set default values for configuration
            conf.setDefault()

        # Notify ConfigurationPanel
        self.onShowConfiguration(conf)
        # Trigger update scenes
        self._deviceDataReceived()

    def handle_deviceSchema(self, deviceId, schema):
        if deviceId not in self.deviceData:
            print('Unrequested schema for device {} arrived'.format(deviceId))
            return

        conf = self.deviceData[deviceId]
        # Schema already existent -> schema injected
        if conf.status in ("alive", "monitoring"):
            get_network().onGetDeviceConfiguration(self.deviceData[deviceId])

        # Add configuration with schema to device data
        conf.setSchema(schema)
        box_signal = conf.boxvalue.state.signalUpdateComponent
        box_signal.connect(handle_device_state_change)

        self.onShowConfiguration(conf)
        # Trigger update scenes
        self._deviceDataReceived()

    def handle_deviceConfiguration(self, deviceId, configuration):
        device = self.deviceData.get(deviceId)
        if device is None or device.descriptor is None:
            return

        with BulkNotifications(device):
            device.fromHash(configuration)
        if device.status == "schema":
            device.status = "alive"
            # Trigger update scenes - to draw possible Workflow Connections
            self._deviceDataReceived()
        if device.status == "alive" and device.visible > 0:
            device.status = "monitoring"

    def handle_propertyHistory(self, deviceId, property, data):
        box = self.deviceData[deviceId].getBox(property.split("."))
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
            return
        d = {'items': reply['items']}  # Hash -> dict
        event = KaraboBroadcastEvent(KaraboEventSender.ProjectItemsLoaded, d)
        broadcast_event(event)

    def handle_projectSaveItems(self, reply):
        # ``reply`` is a Hash containing a list of item hashes
        success = reply.get('success', True)
        if not success:
            MessageBox.showError(reply['reason'])
            return
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
        deviceId, property = name.split(":")
        self.deviceData[deviceId].getBox(
            property.split(".")).boxvalue.schema.fromHash(data)

    def handle_initReply(self, deviceId, success, message):
        device = self.deviceData.get(deviceId)
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

    def _extractRunConfigurators(self):
        """ This method extracts the existing devices of type
            ``RunConfigurator of the ``self.systemHash`` and returns the
            instance ids in a list.
         """
        instanceIds = []
        if 'device' in self.systemHash:
            for deviceId, _, attrs in self.systemHash['device'].iterall():
                classId = attrs.get("classId", "unknown-class")
                if classId == 'RunConfigurator':
                    instanceIds.append(deviceId)
        return instanceIds

    def handle_runConfigSourcesInGroup(self, reply):
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.RunConfigSourcesUpdate, reply))

# ------------------------------------------------------------------

    def get_server_status(self, server_id):
        """Get status string for given server and return it
        """
        try:
            server_key = 'server.{}'.format(server_id)
            attrs = self.systemHash[server_key, ...]
            status = attrs.get('status', 'ok')
        except KeyError:
            status = 'offline'

        return status

    def get_device_status(self, device_id):
        """Get status string for given device and return it
        """
        try:
            device_key = 'device.{}'.format(device_id)
            attrs = self.systemHash[device_key, ...]
            status = attrs.get('status', 'ok')
        except KeyError:
            status = 'offline'

        return status


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
