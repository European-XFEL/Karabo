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
from messagebox import MessageBox
from navigationtreemodel import NavigationTreeModel
from project import ProjectConfiguration
from projectmodel import ProjectModel
from sqldatabase import SqlDatabase

from PyQt4.QtCore import (pyqtSignal, QDir, QFile, QFileInfo, QIODevice, QObject)
from PyQt4.QtGui import (QDialog, QFileDialog, QMessageBox)


class _Manager(QObject):
    # signals
    signalReset = pyqtSignal()
    signalServerConnection = pyqtSignal(bool) # connect?

    signalNewNavigationItem = pyqtSignal(dict) # id, name, type, (status), (refType), (refId), (schema)
    signalSelectNewNavigationItem = pyqtSignal(str) # deviceId
    signalShowConfiguration = pyqtSignal(object) # configuration
    signalDeviceInstanceChanged = pyqtSignal(dict, str)
    signalKillDevice = pyqtSignal(str) # deviceId
    signalKillServer = pyqtSignal(str) # serverId

    signalRefreshInstance = pyqtSignal(object) # deviceId
    signalInitDevice = pyqtSignal(str, object) # deviceId, hash
    signalExecute = pyqtSignal(str, dict) # deviceId, slotName/arguments

    signalReconfigure = pyqtSignal(list)
    signalConflictStateChanged = pyqtSignal(object, bool) # key, hasConflict
    signalChangingState = pyqtSignal(object, bool) # deviceId, isChanging
    signalErrorState = pyqtSignal(object, bool) # deviceId, inErrorState

    signalInstanceGone = pyqtSignal(str, str) # path, parentPath

    signalNewVisibleDevice = pyqtSignal(object) # device
    signalRemoveVisibleDevice = pyqtSignal(str) # deviceId

    signalLogDataAvailable = pyqtSignal(str) # logData
    signalNotificationAvailable = pyqtSignal(str, str, str, str, str) # timestam, type, shortMessage, detailedMessage, deviceId

    signalGetClassSchema = pyqtSignal(str, str) # serverId, classId
    signalGetDeviceSchema = pyqtSignal(str) # deviceId
    signalGetFromPast = pyqtSignal(object, str, str, int) # deviceId, property, t0, t1


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
        self.projectTopology.signalShowProjectConfiguration. \
                        connect(self.onShowConfiguration)
        
        self.systemHash = None

        # Sets all parameters to start configuration
        self.reset()


    # Sets all parameters to start configuration
    def reset(self):
        # Map stores { (serverId, class), Configuration }
        self.serverClassData = dict()
        # Map stores { deviceId, Configuration }
        self.deviceData = dict()

        # Map stores all keys and DataNofiers for editable widgets
        self.__keyNotifierMapEditableValue = dict()
        # Map stores all keys and DataNofiers for display widgets
        self.__keyNotifierMapDisplayValue = dict()
        
        # Dictionary to store instanceId of visible DEVICE_INSTANCEs with counter
        if hasattr(self, "visibleDeviceIdCount"):
            for deviceId in self.visibleDeviceIdCount.keys():
                if self.visibleDeviceIdCount[deviceId] > 0:
                    self.signalRemoveVisibleDevice.emit(deviceId)
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


    def checkSystemHash(self):
        """
        This function checks whether the systemHash is set correctly.
        If not, a signal to connect to the server is emitted.
        """
        if self.systemHash is not None:
            return True

        reply = QMessageBox.question(
            None, "No server connection",
            "There is no connection to the server.<br>"
            "Do you want to establish a server connection?",
            QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

        if reply == QMessageBox.No:
            return False

        # Send signal to establish server connection to projectpanel
        self.signalServerConnection.emit(True)
        return False


    def onServerConnectionChanged(self, isConnected):
        """
        If the server connection is changed, the model needs an update.
        """
        if isConnected:
            return

        self.systemHash = None


    def disconnectedFromServer(self):
        # Reset manager settings
        self.reset()
        # Send reset signal to configurator to clear stacked widget
        self.signalReset.emit()


    def _getDeviceIdFromInternalPath(self, internalPath):
        """
        This function gets as parameter the \internalPath of a device or a device
        class.
        
        Only if this internal path belongs to a device, the deviceId is return,
        otherwise return None.
        """
        splittedPath = internalPath.split('.')
        if len(splittedPath) < 1:
            # No device selected
            return None

        return splittedPath[0]


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


    def onDeviceInstanceValuesChanged(self, boxes):
        self.signalReconfigure.emit(boxes)


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


    def initDevice(self, serverId, classId, config=None):
        if config is None:
            # Use standard configuration for server/classId
            config = self.serverClassData[serverId, classId].toHash()
       
        # Send signal to network
        self.signalInitDevice.emit(serverId, Hash(classId, config))
        self.__isInitDeviceCurrentlyProcessed = True


    def killDevice(self, deviceId):
        reply = QMessageBox.question(None, 'Message',
            "Do you really want to kill the device \"<b>{}</b>\"?".format(deviceId),
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return

        if deviceId in self.deviceData:
            # Remove deviceId data
            del self.deviceData[deviceId]
        
        self.signalKillDevice.emit(deviceId)


    def killServer(self, serverId):
        reply = QMessageBox.question(None, 'Message',
            "Do you really want to kill the device server \"<b>{}</b>\"?".format(serverId),
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return
        
        self.signalKillServer.emit(serverId)


    def selectNavigationItemByKey(self, path):
        self.signalNavigationItemSelectionChanged.emit(path)


    def executeCommand(self, itemInfo):
        # instanceId, name, arguments
        path = itemInfo.get('path')
        keys = path.split('.')
        deviceId = keys[0]
        
        command = itemInfo.get('command')
        args = itemInfo.get('args')
        
        self.signalExecute.emit(deviceId, dict(command=command, args=args))


    def onLogDataAvailable(self, logData):
        # Send message to logging panel
        self.signalLogDataAvailable.emit(logData)


    def onRefreshInstance(self, configuration):
        self.signalRefreshInstance.emit(configuration)

   
    def onNewNavigationItem(self, itemInfo):
        self.signalNewNavigationItem.emit(itemInfo)


    def selectDeviceByPath(self, path):
        if self.__isInitDeviceCurrentlyProcessed is True:
            self.signalSelectNewNavigationItem.emit(path)
            self.__isInitDeviceCurrentlyProcessed = False


    def potentiallyRefreshVisibleDevice(self, deviceId):
        # If deviceId is already visible in scene but was offline, force refresh
        if (deviceId in self.visibleDeviceIdCount) and \
           (self.visibleDeviceIdCount[deviceId] > 0):
            self.signalSelectNewNavigationItem.emit(deviceId)
            self.signalRefreshInstance.emit(deviceId)


    def onShowConfiguration(self, conf):
        # Notify ConfigurationPanel
        self.signalShowConfiguration.emit(conf)


    def onDeviceInstanceChanged(self, itemInfo, xml):
        self.signalDeviceInstanceChanged.emit(itemInfo, xml)


    def currentDeviceId(self):
        """
        This function returns the deviceId of the currently selected device.
        
        Returns None, if deviceId not available.
        """
        if self.systemTopology.currentIndex().isValid():
            indexInfo = self.systemTopology.indexInfo(self.systemTopology.currentIndex())
            return indexInfo.get("deviceId")
        elif self.projectTopology.currentIndex().isValid():
            indexInfo = self.projectTopology.indexInfo(self.projectTopology.currentIndex())
            return indexInfo.get("deviceId")
        else:
            return None


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


    def currentConfigurationAsHash(self):
        """
        This function returns the configuration hash of the currently selected
        device which can be part of the systemTopology or the projectTopology.
        
        Returns None, if no device is selected None.
        """
        conf, classId = self.currentConfigurationAndClassId()
        if conf is None: return None
        
        return Hash(classId, conf.toHash())


    def onOpenFromFile(self):
        filename = QFileDialog.getOpenFileName(None, "Open configuration", \
                                               QDir.tempPath(), "XML (*.xml)")
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

        conf.fromHash(config[classId])


    def onOpenFromProject(self):
        # Open dialog to select project and configuration
        dialog = SelectProjectConfigurationDialog(self.projectTopology.projects)
        if dialog.exec_() == QDialog.Rejected:
            return
        
        conf, classId = self.currentConfigurationAndClassId()
        if not dialog.projectConfiguration().hash.has(classId):
            MessageBox.showError("Configuration open failed")
            return
        
        conf.fromHash(dialog.projectConfiguration().hash[classId])


    def onSaveToFile(self):
        filename = QFileDialog.getSaveFileName(None, "Save configuration as", QDir.tempPath(), "XML (*.xml)")
        if len(filename) < 1:
            return
        
        fi = QFileInfo(filename)
        if len(fi.suffix()) < 1:
            filename += ".xml"
        
        config = self.currentConfigurationAsHash()
        if config is None:
            MessageBox.showError("Configuration save failed")
            return
        
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
        project.addConfiguration(conf.key, ProjectConfiguration(project,
                                                      dialog.configurationName(),
                                                      Hash(classId, conf.toHash())))
        self.projectTopology.updateData()


    def handleSystemTopology(self, config):
        if self.systemHash is None:
            self.systemHash = config
        else:
            self.systemHash.merge(config, "merge")
        # Update navigation and project treemodel
        self.systemTopology.updateData(config)
        for v in self.deviceData.itervalues():
            try:
                attrs = config["device.{}".format(v.key), ...]
                if attrs.get("status") == "error":
                    v.status = "error"
                else:
                    if v.status not in ("requested", "schema",
                                        "error", "alive"):
                        v.status = "online"
                v.classId = attrs.get("classId")
                v.serverId = attrs.get("serverId")
            except KeyError:
                v.status = "offline"
        self.projectTopology.updateNeeded()


    def handleInstanceNew(self, config):
        """
        This function gets the configuration for a new instance.
        Before the system topology is updated, it is checked whether this instance
        already exists in the central hash.
        If \True, the existing stuff is removed (from central hash, old parameter
                  page..
        If \False, nothing is removed.
        """
        
        # Check for existing stuff and remove
        instanceIds = self.systemTopology.removeExistingInstances(config)
        for id in instanceIds:
            timestamp = datetime.now()
            # TODO: better format for timestamp and timestamp generation in karabo
            timestamp = timestamp.strftime("%Y-%m-%d %H:%M:%S")
            logMessage = timestamp + " | " + "INFO" + " | " + id + " | " \
                         "Detected dirty shutdown for instance \"" + id + "\", which " \
                         "is coming up now.#"
            # A log message is triggered
            self.onLogDataAvailable(logMessage)

        # Update system topology with new configuration
        self.handleSystemTopology(config)

        # If device was instantiated from GUI, it should be selected after coming up
        deviceKey = "device"
        if config.has(deviceKey):
            deviceConfig = config.get(deviceKey)
            deviceIds = list()
            deviceConfig.getKeys(deviceIds)
            for deviceId in deviceIds:
                self.selectDeviceByPath(deviceId)
                self.potentiallyRefreshVisibleDevice(deviceId)


    def handleInstanceGone(self, instanceId):
        """
        Remove instanceId from central hash and update
        """
        if instanceId in self.deviceData:
            self.deviceData[instanceId].status = "offline"
        # Update system topology
        parentPath = self.systemTopology.erase(instanceId)
        if parentPath is not None:
            self.signalInstanceGone.emit(instanceId, parentPath)
        path = None
        if self.systemHash.has("server." + instanceId):
            path = "server." + instanceId
        elif self.systemHash.has("device." + instanceId):
            path = "device." + instanceId
        self.systemTopology.erase(path)

        self.projectTopology.updateNeeded()






    def handleClassSchema(self, classInfo):
        serverId = classInfo.get('serverId')
        classId = classInfo.get('classId')
        if (serverId, classId) not in self.serverClassData:
            print 'not requested schema for classId {} arrived'.format(classId)
            return
        
        schema = classInfo.get('schema')
        
        conf = self.serverClassData[serverId, classId]
        conf.setSchema(schema)
        # Set default values for configuration
        conf.setDefault()
        # Notify ConfigurationPanel
        self.onShowConfiguration(conf)


    def getClass(self, serverId, classId):
        if (serverId, classId) not in self.serverClassData:
            path = "{}.{}".format(serverId, classId)
            self.serverClassData[serverId, classId] = Configuration(path,
                                                                    'class')
            self.signalGetClassSchema.emit(serverId, classId)
        return self.serverClassData[serverId, classId]
    
    
    def handleDeviceSchema(self, instanceInfo):
        deviceId = instanceInfo['deviceId']
        if deviceId not in self.deviceData:
            print 'not requested schema for device {} arrived'.format(deviceId)
            return
        
        # Add configuration with schema to device data
        schema = instanceInfo['schema']
        conf = self.deviceData[deviceId]
        conf.setSchema(schema)
        conf.value.state.signalUpdateComponent.connect(
            self._triggerStateChange)
        
        self.onShowConfiguration(conf)


    def getDevice(self, deviceId):
        c = self.deviceData.get(deviceId)
        if c is None:
            c = self.deviceData[deviceId] = Configuration(deviceId, 'device')
        if c.descriptor is None and c.status != "requested":
            self.signalGetDeviceSchema.emit(deviceId)
            c.status = "requested"
        return c


    def handleDeviceSchemaUpdated(self, instanceInfo):
        deviceId = instanceInfo.get("deviceId")
        if deviceId in self.deviceData:
            self.deviceData[deviceId].schema = None
        
        self.handleDeviceSchema(instanceInfo)
        self.onRefreshInstance(deviceId)


    # TODO: This function must be thread-safe!!
    def handleConfigurationChanged(self, instanceInfo):
        deviceId = instanceInfo.get("deviceId")
        if self.deviceData.get(deviceId) is None: return
        
        config = instanceInfo.get("configuration")
        self.deviceData[deviceId].fromHash(config)


    def handleHistoricData(self, hash):
        box = self.deviceData[hash["deviceId"]].getBox(
            hash["property"].split("."))
        box.signalHistoricData.emit(box, hash["data"])


    def handleNotification(self, instanceInfo):
        deviceId = instanceInfo.get("deviceId")
        timestamp = datetime.now()
        # TODO: better format for timestamp and timestamp generation in karabo
        timestamp = timestamp.strftime("%Y-%m-%d %H:%M:%S")
        messageType = instanceInfo.get("messageType")
        shortMsg = instanceInfo.get("shortMsg")
        detailedMsg = instanceInfo.get("detailedMsg")
        
        self.signalNotificationAvailable.emit(timestamp, messageType, shortMsg, detailedMsg, deviceId)


manager = _Manager()

def Manager():
    return manager
