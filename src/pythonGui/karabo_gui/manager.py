
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

from karabo_gui.configuration import BulkNotifications
from karabo_gui.dialogs.configurationdialog import SelectProjectDialog, SelectProjectConfigurationDialog
from datetime import datetime
from karabo.middlelayer import Hash, Schema, XMLWriter, XMLParser, ProjectConfiguration
import karabo_gui.globals as globals
from karabo_gui.messagebox import MessageBox
from karabo_gui.navigationtreemodel import NavigationTreeModel
from karabo_gui.network import Network
from karabo_gui.projectmodel import ProjectModel
from karabo_gui.topology import getClass
from karabo_gui.util import getSaveFileName, getSchemaModifiedAttrs

from PyQt4.QtCore import pyqtSignal, QObject
from PyQt4.QtGui import (QDialog, QFileDialog, QMessageBox)


class _Manager(QObject):
    # signals
    signalReset = pyqtSignal()
    signalUpdateScenes = pyqtSignal()

    signalNewNavigationItem = pyqtSignal(dict) # id, name, type, (status), (refType), (refId), (schema)
    signalSelectNewNavigationItem = pyqtSignal(str) # deviceId
    signalShowConfiguration = pyqtSignal(object) # configuration

    signalConflictStateChanged = pyqtSignal(object, bool) # key, hasConflict
    signalChangingState = pyqtSignal(object, bool) # deviceId, isChanging
    signalErrorState = pyqtSignal(object, bool) # deviceId, inErrorState

    signalLogDataAvailable = pyqtSignal(object) # logData
    signalNotificationAvailable = pyqtSignal(str, str, str, str)
    
    signalAvailableProjects = pyqtSignal(object) # hash of projects and attributes
    signalProjectLoaded = pyqtSignal(str, object, object) # projectName, metaData, data
    signalProjectSaved = pyqtSignal(str, bool, object) # projectName, success, data


    def __init__(self, *args, **kwargs):
        super(_Manager, self).__init__()

        # Model for navigation views
        self.systemTopology = NavigationTreeModel(self)
        self.systemTopology.selectionModel.selectionChanged. \
                        connect(self.onNavigationTreeModelSelectionChanged)
        # Model for project views
        self.projectTopology = ProjectModel(self)
        self.projectTopology.selectionModel.selectionChanged. \
                        connect(self.onProjectModelSelectionChanged)

        Network().signalServerConnectionChanged.connect(
            self.onServerConnectionChanged)
        Network().signalReceivedData.connect(self.onReceivedData)

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


    def initDevice(self, serverId, classId, deviceId, config=None):
        if config is None:
            # Use standard configuration for server/classId
            conf = self.serverClassData.get((serverId, classId))
            if conf is not None:
                config, _ = conf.toHash()  # Ignore returned attributes
            else:
                config = Hash()

        # Compute a runtime schema from the configuration and an unmodified
        # copy of the device class schema.
        baseSchema = self._immutableServerClassData[serverId, classId]
        schemaAttrs = getSchemaModifiedAttrs(baseSchema, config)

        # Send signal to network
        Network().onInitDevice(serverId, classId, deviceId, config,
                               schemaAttrs=schemaAttrs)

    def shutdownDevice(self, deviceId, showConfirm=True):
        if showConfirm:
            reply = QMessageBox.question(None, 'Shutdown device',
                "Do you really want to shutdown the device \"<b>{}</b>\"?".format(deviceId),
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                return

        Network().onKillDevice(deviceId)


    def shutdownServer(self, serverId):
        reply = QMessageBox.question(None, 'Shutdown device server',
            "Do you really want to shutdown the device server \"<b>{}</b>\"?".format(serverId),
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return

        Network().onKillServer(serverId)


    def onReceivedData(self, hash):
        getattr(self, "handle_" + hash["type"])(
            **{k: v for k, v in hash.items() if k != "type"})


    def onServerConnectionChanged(self, isConnected):
        """
        If the server connection is changed, the model needs an update.
        """
        if isConnected:
            return

        # Send reset signal to all panels which needs a reset
        self.signalReset.emit()
        # Reset manager settings
        self.reset()


    def _triggerStateChange(self, box, value, timestamp):
        configuration = box.configuration
        # Update GUI due to state changes
        if value == "Changing...":
            self.signalChangingState.emit(configuration, True)
        else:
            if ("Error" in value) or ("error" in value):
                self.signalErrorState.emit(configuration, True)
            else:
                self.signalErrorState.emit(configuration, False)
            
            self.signalChangingState.emit(configuration, False)


    def onConflictStateChanged(self, key, hasConflict):
        self.signalConflictStateChanged.emit(key, hasConflict)


    def onNavigationTreeModelSelectionChanged(self, selected, deselect):
        """
        This slot is called whenever something of the navigation panel is selected.
        If an item was selected, the selection of the project panel is cleared.
        """
        if not selected.indexes():
            return

        self.projectTopology.selectionModel.clear()


    def onProjectModelSelectionChanged(self, selected, deselected):
        """
        This slot is called whenever something of the project panel is selected.
        If an item was selected, the selection of the navigation panel is cleared.
        """
        if not selected.indexes():
            return

        self.systemTopology.selectionModel.clear()


    def onNewNavigationItem(self, itemInfo):
        self.signalNewNavigationItem.emit(itemInfo)


    def onShowConfiguration(self, conf):
        # Notify ConfigurationPanel
        self.signalShowConfiguration.emit(conf)


    def currentConfigurationAndClassId(self):
        """
        This function returns the configuration of the currently selected device
        which can be part of the systemTopology or the projectTopology.
        
        Returns None, If no device is selected.
        """
        if self.systemTopology.currentIndex().isValid():
            indexInfo = self.systemTopology.indexInfo(self.systemTopology.currentIndex())
            deviceId = indexInfo.get("deviceId")
            classId = indexInfo.get("classId")
            serverId = indexInfo.get("serverId")

            if deviceId is not None and deviceId:
                return self.deviceData[deviceId], classId
            elif serverId is not None:
                return self.serverClassData[serverId, classId], classId
            else:
                return None
        elif self.projectTopology.currentIndex().isValid():
            indexInfo = self.projectTopology.indexInfo(self.projectTopology.currentIndex())
            return indexInfo.get("conf"), indexInfo.get("classId")
        else:
            return None

    @staticmethod
    def _loadClassConfiguration(classConfig, configHash, classId):
        if classId not in configHash:
            MessageBox.showError("Configuration load failed")
            return
        classConfig.fromHash(configHash[classId])

    def onOpenFromFile(self):
        filename = QFileDialog.getOpenFileName(None, "Open configuration", \
                                               globals.HIDDEN_KARABO_FOLDER,
                                               "XML (*.xml)")
        if not filename:
            return

        conf, classId = self.currentConfigurationAndClassId()
        
        r = XMLParser()
        with open(filename, 'rb') as file:
            config = r.read(file.read())

        self._loadClassConfiguration(conf, config, classId)

    def onOpenFromProject(self):
        # Open dialog to select project and configuration
        dialog = SelectProjectConfigurationDialog(self.projectTopology.projects)
        if dialog.exec_() == QDialog.Rejected:
            return
        
        conf, classId = self.currentConfigurationAndClassId()

        config = dialog.projectConfiguration().hash
        self._loadClassConfiguration(conf, config, classId)

    def onSaveToFile(self):
        """
        This function saves the current configuration of a device to a file.
        """
        filename = getSaveFileName(
            "Save configuration as", globals.HIDDEN_KARABO_FOLDER,
            "Configuration (*.xml)", "xml")
        if not filename:
            return

        conf, classId = self.currentConfigurationAndClassId()
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


    def onSaveToProject(self):
        """
        This function saves the current configuration of a device to the project.
        """
        # Open dialog to select project to which configuration should be saved
        dialog = SelectProjectDialog(self.projectTopology.projects)
        if dialog.exec_() == QDialog.Rejected:
            return

        project = dialog.selectedProject()
        conf, classId = self.currentConfigurationAndClassId()
        
        # Check, if another configuration with same name already exists
        name = dialog.configurationName()
        if name.endswith(".xml"):
            name = name[:-4]
        
        overwrite = False
        for deviceId, configs in project.configurations.items():
            if deviceId == conf.id:
                for c in configs:
                    if c.filename[:-4] == name:
                        reply = QMessageBox.question(None, 'Project configuration already exists',
                            "Another configuration with the same name \"<b>{}</b>\" <br> "
                            "already exists. Do you want to overwrite it?".format(name),
                            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

                        if reply == QMessageBox.No:
                            return

                        overwrite = True
                        break
            
            if overwrite:
                # Overwrite existing device
                index = project.removeConfiguration(deviceId, c)
                hsh, attrs = conf.toHash()
                confHash = Hash(classId, hsh)
                confHash[classId, ...] = attrs
                project.insertConfiguration(index, conf.id,
                                            ProjectConfiguration(project, name,
                                            confHash))
                return

        hsh, attrs = conf.toHash()
        confHash = Hash(classId, hsh)
        confHash[classId, ...] = attrs
        project.addConfiguration(conf.id, ProjectConfiguration(project, name,
                                          confHash))


    def handle_log(self, messages):
        self.signalLogDataAvailable.emit(messages)


    def handle_brokerInformation(self, **kwargs):
        Network()._handleBrokerInformation(**kwargs)


    def handle_systemTopology(self, systemTopology):
        if self.systemHash is None:
            self.systemHash = systemTopology
        else:
            self.systemHash.merge(systemTopology, "merge")
        # Update navigation and project treemodel
        self.systemTopology.updateData(systemTopology)
        for v in self.deviceData.values():
            v.updateStatus()
        self.projectTopology.updateNeeded()

    def handle_systemVersion(self, version):
        """ Handle the version number reply from the GUI server.
        """
        pass

    def handle_instanceNew(self, topologyEntry):
        """ This function gets the configuration for a new instance.

        If the instance already exists in the central hash, it is first
        removed from there. """
        # Check for existing stuff and remove
        instanceIds, serverClassIds = self.systemTopology.detectExistingInstances(topologyEntry)
        for id in instanceIds:
            logMessage = dict(
                timestamp=datetime.now().isoformat(), type="INFO", category=id,
                message='Detected dirty shutdown for instance "{}", '
                        'which is coming up now.'.format(id))
            self.handle_log([logMessage])
            
            # Clear deviceId parameter page, if existent
            self._clearDeviceParameterPage(id)

        self._clearServerClassParameterPages(serverClassIds)

        # Update system topology with new configuration
        self.handle_instanceUpdated(topologyEntry)

    def handle_instanceUpdated(self, topologyEntry):
        self.handle_systemTopology(topologyEntry)
        # Topology changed so send new class schema requests
        if "server" in topologyEntry:
            # Request schema for already viewed classes, if a server is new
            for k in self.serverClassData:
                getClass(k[0], k[1])


    def handle_instanceGone(self, instanceId, instanceType):
        """ Remove instanceId from central hash and update """
        
        if instanceType in ("device", "macro"):
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

            # Remove device from systemHash
            path = instanceType + "." + instanceId
            if self.systemHash is not None and path in self.systemHash:
                del self.systemHash[path]
        elif instanceType == "server":
            # Update system topology
            serverClassIds = self.systemTopology.eraseServer(instanceId)
            self._clearServerClassParameterPages(serverClassIds)
            
            # Remove server from systemHash
            path = "server." + instanceId
            if self.systemHash is not None and path in self.systemHash:
                del self.systemHash[path]
            
            for v in self.deviceData.values():
                v.updateStatus()

            # Clear corresponding parameter pages
            self.projectTopology.clearParameterPages(serverClassIds)
        
        self.projectTopology.updateNeeded()


    def handle_classSchema(self, serverId, classId, schema):
        if (serverId, classId) not in self.serverClassData:
            print('not requested schema for classId {} arrived'.format(classId))
            return

        # Save a clean copy
        schemaCopy = Schema()
        schemaCopy.copy(schema)
        self._immutableServerClassData[serverId, classId] = schemaCopy

        conf = self.serverClassData[serverId, classId]
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
        self.signalUpdateScenes.emit()


    def handle_deviceSchema(self, deviceId, schema):
        if deviceId not in self.deviceData:
            print('not requested schema for device {} arrived'.format(deviceId))
            return
        
        conf = self.deviceData[deviceId]
        # Schema already existent -> schema injected
        if conf.status in ("alive", "monitoring"):
            Network().onGetDeviceConfiguration(self.deviceData[deviceId])
        
        # Add configuration with schema to device data
        conf.setSchema(schema)
        conf.boxvalue.state.signalUpdateComponent.connect(
            self._triggerStateChange)

        self.onShowConfiguration(conf)
        # Trigger update scenes
        self.signalUpdateScenes.emit()


    def handle_deviceConfiguration(self, deviceId, configuration):
        device = self.deviceData.get(deviceId)
        if device is None or device.descriptor is None:
            return

        with BulkNotifications(device):
            device.fromHash(configuration)
        if device.status == "schema":
            device.status = "alive"
            # Trigger update scenes - to draw possible Workflow Connections
            self.signalUpdateScenes.emit()
        if device.status == "alive" and device.visible > 0:
            device.status = "monitoring"


    def handle_propertyHistory(self, deviceId, property, data):
        box = self.deviceData[deviceId].getBox(property.split("."))
        box.signalHistoricData.emit(box, data)


    def handle_availableProjects(self, availableProjects):
        self.signalAvailableProjects.emit(availableProjects)


    def handle_projectNew(self, name, success, data):
        self.signalProjectSaved.emit(name, success, data)


    def handle_projectLoaded(self, name, metaData, buffer):
        self.signalProjectLoaded.emit(name, metaData, buffer)
            

    def handle_projectSaved(self, name, success, data):
        self.signalProjectSaved.emit(name, success, data)


    def handle_projectClosed(self, name, success, data):
        """ exists for compatibility with old GuiServers only """
        pass


    def handle_notification(self, deviceId, messageType, shortMsg, detailedMsg):
        self.signalNotificationAvailable.emit(deviceId, messageType, shortMsg,
                                              detailedMsg)

    def handle_networkData(self, name, data):
        deviceId, property = name.split(":")
        self.deviceData[deviceId].getBox(
            property.split(".")).boxvalue.schema.fromHash(data)

    def handle_initReply(self, deviceId, success, message):
        device = self.deviceData.get(deviceId)
        if device is not None:
            device.signalInitReply.emit(success, message)
