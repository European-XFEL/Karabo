from __future__ import unicode_literals
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

__all__ = ["Manager"]


from configuration import Configuration
from dialogs.configurationdialog import SelectProjectDialog, SelectProjectConfigurationDialog
from datetime import datetime
from karabo.hash import Hash, XMLWriter, XMLParser
import globals
from messagebox import MessageBox
from navigationtreemodel import NavigationTreeModel
from network import Network
from project import ProjectConfiguration
from projectmodel import ProjectModel
from sqldatabase import SqlDatabase

from PyQt4.QtCore import (pyqtSignal, QDir, QFile, QFileInfo, QIODevice, QObject)
from PyQt4.QtGui import (QDialog, QFileDialog, QMessageBox)


class _Manager(QObject):
    # signals
    signalReset = pyqtSignal()
    signalShowEmptyConfigurationPage = pyqtSignal()

    signalNewNavigationItem = pyqtSignal(dict) # id, name, type, (status), (refType), (refId), (schema)
    signalSelectNewNavigationItem = pyqtSignal(str) # deviceId
    signalShowConfiguration = pyqtSignal(object) # configuration

    signalConflictStateChanged = pyqtSignal(object, bool) # key, hasConflict
    signalChangingState = pyqtSignal(object, bool) # deviceId, isChanging
    signalErrorState = pyqtSignal(object, bool) # deviceId, inErrorState

    signalLogDataAvailable = pyqtSignal(str) # logData
    signalNotificationAvailable = pyqtSignal(str, str, str, str, str) # timestam, type, shortMessage, detailedMessage, deviceId

    def __init__(self, *args, **kwargs):
        super(_Manager, self).__init__()
        
        # Initiate database connection
        self.sqlDatabase = SqlDatabase()
        self.sqlDatabase.openConnection()
        
        # Model for navigation views
        self.systemTopology = NavigationTreeModel(self)
        self.systemTopology.selectionModel.selectionChanged. \
                        connect(self.onNavigationTreeModelSelectionChanged)
        # Model for project views
        self.projectTopology = ProjectModel(self)
        self.projectTopology.selectionModel.selectionChanged. \
                        connect(self.onProjectModelSelectionChanged)

        self.systemHash = None

        Network().signalServerConnectionChanged.connect(
            self.onServerConnectionChanged)
        Network().signalServerConnectionChanged.connect(
            self.systemTopology.onServerConnectionChanged)
        Network().signalReceivedData.connect(self.onReceivedData)

        # Sets all parameters to start configuration
        self.reset()


    # Sets all parameters to start configuration
    def reset(self):
        # Map stores { (serverId, class), Configuration }
        self.serverClassData = dict()
        # Map stores { deviceId, Configuration }
        self.deviceData = dict()
        
        # Dictionary to store instanceId of visible DEVICE_INSTANCEs with counter
        if hasattr(self, "visibleDeviceIdCount"):
            for deviceId in self.visibleDeviceIdCount.keys():
                if self.visibleDeviceIdCount[deviceId] > 0:
                    Network().onRemoveVisibleDevice(deviceId)
        self.visibleDeviceIdCount = dict()
        
        # State, if initiate device is currently processed
        self.__isInitDeviceCurrentlyProcessed = False


    def closeDatabaseConnection(self):
        self.sqlDatabase.closeConnection()

    
    def _changeClassData(self, key, value):
        serverId, classId, property = key.split(".", 2)
        
        if "@" in property:
            # Merge attribute value
            propertyKey, attributeKey = property.split("@")
            self.serverClassData[serverId, classId].setAttribute(propertyKey, attributeKey, value)
        else:
            self.serverClassData[serverId, classId].set(property, value)


    def _handleSystemTopology(self, config):
        if self.systemHash is None:
            self.systemHash = config
        else:
            self.systemHash.merge(config, "merge")
        # Update navigation and project treemodel
        self.systemTopology.updateData(config)
        for v in self.deviceData.itervalues():
            v.updateStatus()

        self.projectTopology.updateNeeded()


    def _handleLogData(self, logMessage):
        self.signalLogDataAvailable.emit(logMessage)


    def _clearServerClassParameterPage(self, serverClassIds):
        for serverClassId in serverClassIds:
            try:
                conf = self.serverClassData[serverClassId]
                # Clear corresponding parameter page
                if conf.parameterEditor is not None:
                    conf.parameterEditor.clear()
                
                if conf.descriptor is not None:
                    conf.redummy()
            except KeyError:
                pass


    def initDevice(self, serverId, classId, config=None):
        if config is None:
            # Use standard configuration for server/classId
            config = self.serverClassData[serverId, classId].toHash()
       
        # Send signal to network
        Network().onInitDevice(serverId, Hash(classId, config))
        self.__isInitDeviceCurrentlyProcessed = True


    def killDevice(self, deviceId):
        reply = QMessageBox.question(None, 'Message',
            "Do you really want to kill the device \"<b>{}</b>\"?".format(deviceId),
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return

        Network().onKillDevice(deviceId)


    def killServer(self, serverId):
        reply = QMessageBox.question(None, 'Message',
            "Do you really want to kill the device server \"<b>{}</b>\"?".format(serverId),
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return

        Network().onKillServer(serverId)


    def onReceivedData(self, hash):
        getattr(self, "handle_" + hash["type"])(hash)


    def onServerConnectionChanged(self, isConnected):
        """
        If the server connection is changed, the model needs an update.
        """
        if isConnected:
            return

        self.systemHash = None

        # Reset manager settings
        self.reset()
        # Send reset signal to configurator to clear stacked widget
        self.signalReset.emit()


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

            if deviceId is not None:
                return self.deviceData[deviceId], classId
            elif serverId is not None:
                return self.serverClassData[serverId, classId], classId
            else:
                return None
        elif self.projectTopology.currentIndex().isValid():
            indexInfo = self.projectTopology.indexInfo(self.projectTopology.currentIndex())
            return self.projectTopology.currentDevice(), indexInfo.get("classId")
        else:
            return None


    def onOpenFromFile(self):
        filename = QFileDialog.getOpenFileName(None, "Open configuration", \
                                               globals.HIDDEN_KARABO_FOLDER,
                                               "XML (*.xml)")
        if len(filename) < 1:
            return
        
        file = QFile(filename)
        if file.open(QIODevice.ReadOnly | QIODevice.Text) == False:
            return
        
        conf, classId = self.currentConfigurationAndClassId()
        
        r = XMLParser()
        with open(filename, 'r') as file:
            config = r.read(file.read())
        
        if not config.has(classId):
            MessageBox.showError("Configuration open failed")
            return

        conf.dispatchUserChanges(config[classId])


    def onOpenFromProject(self):
        # Open dialog to select project and configuration
        dialog = SelectProjectConfigurationDialog(self.projectTopology.projects)
        if dialog.exec_() == QDialog.Rejected:
            return
        
        conf, classId = self.currentConfigurationAndClassId()
        if not dialog.projectConfiguration().hash.has(classId):
            MessageBox.showError("Configuration open failed")
            return
        
        conf.dispatchUserChanges(dialog.projectConfiguration().hash[classId])


    def onSaveToFile(self):
        filename = QFileDialog.getSaveFileName(None, "Save configuration as",
                                               globals.HIDDEN_KARABO_FOLDER,
                                               "XML (*.xml)")
        if not filename:
            return

        fi = QFileInfo(filename)
        if len(fi.suffix()) < 1:
            filename += ".xml"


        conf, classId = self.currentConfigurationAndClassId()
        if conf is None:
            MessageBox.showError("Configuration save failed")
            return
        config = Hash(classId, conf.toHash())

        # Save configuration to file
        w = XMLWriter()
        with open(filename, 'w') as file:
            w.writeToFile(config, file)


    def onSaveToProject(self):
        # Open dialog to select project to which configuration should be saved
        dialog = SelectProjectDialog(self.projectTopology.projects)
        if dialog.exec_() == QDialog.Rejected:
            return

        project = dialog.selectedProject()
        conf, classId = self.currentConfigurationAndClassId()
        # Add configuration to project
        project.addConfiguration(conf.id, ProjectConfiguration(project,
                                                dialog.configurationName(),
                                                Hash(classId, conf.toHash())))
        self.projectTopology.updateData()


    def handle_log(self, instanceInfo):
        self._handleLogData(instanceInfo["message"])


    def handle_brokerInformation(self, instanceInfo):
        Network()._handleBrokerInformation(instanceInfo)


    def handle_systemTopology(self, instanceInfo):        
        self._handleSystemTopology(instanceInfo.get("systemTopology"))


    def handle_instanceNew(self, instanceInfo):
        """
        This function gets the configuration for a new instance.
        Before the system topology is updated, it is checked whether this instance
        already exists in the central hash.
        If \True, the existing stuff is removed (from central hash, old parameter
                  page..
        If \False, nothing is removed.
        """
        config = instanceInfo.get("topologyEntry")
        # Check for existing stuff and remove
        instanceIds, serverClassIds = self.systemTopology.detectExistingInstances(config)
        for id in instanceIds:
            timestamp = datetime.now()
            # TODO: better format for timestamp and timestamp generation in karabo
            timestamp = timestamp.strftime("%Y-%m-%d %H:%M:%S")
            logMessage = timestamp + " | " + "INFO" + " | " + id + " | " \
                         "Detected dirty shutdown for instance \"" + id + "\", which " \
                         "is coming up now.#"
            # A log message is triggered
            self._handleLogData(logMessage)

        self._clearServerClassParameterPage(serverClassIds)

        # Update system topology with new configuration
        self._handleSystemTopology(config)
        
        if config.has("server"):
            # Request schema for already viewed classes, if a server is new
            for k in self.serverClassData.keys():
                getClass(k[0], k[1])

        # If device was instantiated from GUI, it should be selected after coming up
        deviceConfig = config.get("device")
        if deviceConfig is not None:
            for deviceId in deviceConfig:
                if self.__isInitDeviceCurrentlyProcessed:
                    self.signalSelectNewNavigationItem.emit(deviceId)
                    self.__isInitDeviceCurrentlyProcessed = False


    def handle_instanceUpdated(self, instanceInfo):
        self._handleSystemTopology(instanceInfo.get("topologyEntry"))


    def handle_instanceGone(self, hash):
        """
        Remove instanceId from central hash and update
        """
        instanceId = hash.get("instanceId")
        instanceType = hash.get("instanceType")
        
        if instanceType == "device":
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
            path = "device." + instanceId
            if self.systemHash is not None and path in self.systemHash:
                del self.systemHash[path]
        elif instanceType == "server":
            # Update system topology
            serverClassIds = self.systemTopology.eraseServer(instanceId)
            self._clearServerClassParameterPage(serverClassIds)
            
            # Remove server from systemHash
            path = "server." + instanceId
            if self.systemHash is not None and path in self.systemHash:
                del self.systemHash[path]
            
            for v in self.deviceData.itervalues():
                v.updateStatus()

            # Clear corresponding parameter pages
            self.projectTopology.clearParameterPages(serverClassIds)
        
        # Send signal to Configurator to show nothing
        self.signalShowEmptyConfigurationPage.emit()
        
        self.projectTopology.updateNeeded()


    def handle_classSchema(self, classInfo):
        serverId = classInfo.get('serverId')
        classId = classInfo.get('classId')
        if (serverId, classId) not in self.serverClassData:
            print 'not requested schema for classId {} arrived'.format(classId)
            return
        
        schema = classInfo.get('schema')

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


    def handle_deviceSchema(self, hash):
        deviceId = hash['deviceId']
        if deviceId not in self.deviceData:
            print 'not requested schema for device {} arrived'.format(deviceId)
            return
        
        # Add configuration with schema to device data
        schema = hash['schema']

        conf = self.deviceData[deviceId]
        conf.setSchema(schema)
        conf.value.state.signalUpdateComponent.connect(
            self._triggerStateChange)        
        
        self.onShowConfiguration(conf)


    def handle_schemaUpdated(self, hash):
        deviceId = hash.get("deviceId")
        if deviceId in self.deviceData:
            self.deviceData[deviceId].schema = None

        self.handle_deviceSchema(hash)
        #Network().onRefreshInstance(self.deviceData[deviceId])


    def handle_configurationChanged(self, instanceInfo):
        deviceId = instanceInfo.get("deviceId")
        device = self.deviceData.get(deviceId)
        if device is None or device.descriptor is None:
            return

        config = instanceInfo.get("configuration")
        device.fromHash(config)
        if device.status == "schema":
            device.status = "alive"


    def handle_propertyHistory(self, hash):
        box = self.deviceData[hash["deviceId"]].getBox(
            hash["property"].split("."))
        box.signalHistoricData.emit(box, hash["data"])


    def handle_notification(self, instanceInfo):
        deviceId = instanceInfo.get("deviceId")
        timestamp = datetime.now()
        # TODO: better format for timestamp and timestamp generation in karabo
        timestamp = timestamp.strftime("%Y-%m-%d %H:%M:%S")
        messageType = instanceInfo.get("messageType")
        shortMsg = instanceInfo.get("shortMsg")
        detailedMsg = instanceInfo.get("detailedMsg")
        
        self.signalNotificationAvailable.emit(timestamp, messageType, shortMsg, detailedMsg, deviceId)


def getDevice(deviceId):
    c = manager.deviceData.get(deviceId)
    if c is None:
        c = manager.deviceData[deviceId] = Configuration(deviceId, 'device')
        c.updateStatus()
    if c.descriptor is None and c.status not in ("offline", "requested"):
        Network().onGetDeviceSchema(deviceId)
        c.status = "requested"
    return c


def getClass(serverId, classId):
    c = manager.serverClassData.get((serverId, classId))
    if c is None:
        path = "{}.{}".format(serverId, classId)
        c = manager.serverClassData[serverId, classId] = Configuration(path, 'class')

    if c.descriptor is None or c.status != "requested":
        Network().onGetClassSchema(serverId, classId)
        c.status = "requested"
    return c


manager = _Manager()

def Manager():
    return manager
