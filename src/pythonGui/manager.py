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
import datetime
from enums import NavigationItemTypes
from enums import ConfigChangeTypes
import globals
from karabo.karathon import (Hash, loadFromFile, saveToFile, Timestamp)
from navigationhierarchymodel import NavigationHierarchyModel
from sqldatabase import SqlDatabase

from PyQt4.QtCore import pyqtSignal, QDir, QFile, QFileInfo, QIODevice, QObject
from PyQt4.QtGui import (QFileDialog, QMessageBox)


class DataNotifier(QObject):
    signalUpdateComponent = pyqtSignal(str, object, object) # internalKey, value, timestamp
    signalUpdateDisplayValue = pyqtSignal(str, object, object)
    signalHistoricData = pyqtSignal(str, object)


    def __init__(self, key, component):
        super(DataNotifier, self).__init__()

        self.signalUpdateComponent.connect(self.onValueChanged)
        self.signalUpdateDisplayValue.connect(self.onValueChanged)
        self.addComponent(key, component)


    def onValueChanged(self, key, value, timestamp=None):
        self.value = value
        self.timestamp = timestamp


    def addComponent(self, key, component):
        self.signalUpdateComponent.connect(component.onValueChanged)
        self.signalUpdateDisplayValue.connect(component.onDisplayValueChanged)
        if hasattr(self, "value"):
            self.signalUpdateComponent.emit(key, self.value, self.timestamp)
            self.signalUpdateDisplayValue.emit(key, self.value, self.timestamp)


    def updateDisplayValue(self, key, value, timestamp):
        self.signalUpdateDisplayValue.emit(key, value, timestamp)


class _Manager(QObject):
    # signals
    signalGlobalAccessLevelChanged = pyqtSignal()

    signalReset = pyqtSignal()
    signalInstanceNewReset = pyqtSignal(str) # path

    signalNewNavigationItem = pyqtSignal(dict) # id, name, type, (status), (refType), (refId), (schema)
    signalSelectNewNavigationItem = pyqtSignal(str) # deviceId
    signalSchemaAvailable = pyqtSignal(dict) # key, schema
    signalDeviceInstanceChanged = pyqtSignal(dict, str)
    signalKillDevice = pyqtSignal(str) # deviceId
    signalKillServer = pyqtSignal(str) # serverId
    signalDeviceSchemaUpdated = pyqtSignal(str) # deviceId

    signalRefreshInstance = pyqtSignal(str) # deviceId
    signalInitDevice = pyqtSignal(str, object) # deviceId, hash
    signalExecute = pyqtSignal(str, dict) # deviceId, slotName/arguments

    signalReconfigure = pyqtSignal(str, str, object) # deviceId, attributeId, attributeValue
    signalReconfigureAsHash = pyqtSignal(str, object) # deviceId, hash
    signalDeviceStateChanged = pyqtSignal(str, str) # fullDeviceKey, state
    signalConflictStateChanged = pyqtSignal(str, bool) # key, hasConflict
    signalChangingState = pyqtSignal(str, bool) # deviceId, isChanging
    signalErrorState = pyqtSignal(str, bool) # deviceId, inErrorState

    signalInstanceGone = pyqtSignal(str, str) # path, parentPath

    signalNewVisibleDevice = pyqtSignal(str) # deviceId
    signalRemoveVisibleDevice = pyqtSignal(str) # deviceId

    signalLogDataAvailable = pyqtSignal(str) # logData
    signalNotificationAvailable = pyqtSignal(str, str, str, str, str) # timestam, type, shortMessage, detailedMessage, deviceId

    signalCreateNewProjectConfig = pyqtSignal(object, str, str) # customItem, path, configName
    signalProjectItemChanged = pyqtSignal(dict) # path

    signalGetClassSchema = pyqtSignal(str, str) # serverId, classId
    signalGetDeviceSchema = pyqtSignal(str) # deviceId
    signalGetFromPast = pyqtSignal(str, str, str, str)


    def __init__(self, *args, **kwargs):
        super(_Manager, self).__init__()
        
        # Map stores all keys and DataNofiers for editable widgets
        self.__keyNotifierMapEditableValue = dict()
        # Map stores all keys and DataNofiers for display widgets
        self.__keyNotifierMapDisplayValue = dict()
        
        # Initiate database connection
        self.sqlDatabase = SqlDatabase()
        self.sqlDatabase.openConnection()
        
        # Model for navigationtreeview
        self.systemTopology = NavigationHierarchyModel()
        # Model for projecttree TODO: after merge of projectPanel-branch
        #self.projectTopology = ProjectModel()
        
        # Map stores { (serverId, class), startupConfiguration }
        self.serverClassData = dict()
        # Map stores { deviceId, deviceConfiguration }
        self.deviceData = dict()
        
        # Sets all parameters to start configuration
        self.reset()


    # Sets all parameters to start configuration
    def reset(self):
        # Project hash
        self.__projectHash = Hash("project", Hash(), "project.devices", Hash())
        self.__projectHash.setAttribute("project", "name", "xfelTest")
        self.__projectArrayIndices = []
        
        # Unregister all editable DataNotifiers, if available
        #for key in self.__keyNotifierMapEditableValue:
        #    dataNotifier = self.__keyNotifierMapEditableValue.get(key)
        #    if dataNotifier:
        #        dataNotifier.removeComponents(key)
        # Map stores all keys and DataNofiers for editable widgets
        self.__keyNotifierMapEditableValue = dict()
        
        # Unregister all display DataNotifiers, if available
        #for key in self.__keyNotifierMapDisplayValue:
        #    dataNotifier = self.__keyNotifierMapDisplayValue.get(key)
        #    if dataNotifier:
        #        dataNotifier.removeComponents(key)
        # Map stores all keys and DataNofiers for display widgets
        self.__keyNotifierMapDisplayValue = dict()
        
        # Dictionary to store instanceId of visible DEVICE_INSTANCEs with counter
        self.__visibleDevInsKeys = dict()
        
        # State, if initiate device is currently processed
        self.__isInitDeviceCurrentlyProcessed = False


    def closeDatabaseConnection(self):
        self.sqlDatabase.closeConnection()

    
    def _getDataNotifierEditableValue(self, key):
        return self.__keyNotifierMapEditableValue.get(key)


    def _getDataNotifierDisplayValue(self, key):
        return self.__keyNotifierMapDisplayValue.get(key)


    def _changeClassData(self, key, value):
        serverClassIdParamKey = key.split(".")
        if len(serverClassIdParamKey) < 3:
            return
        
        serverId = serverClassIdParamKey[0]
        classId = serverClassIdParamKey[1]
        paramKey = serverClassIdParamKey[2]
        
        if "@" in paramKey:
            # Merge attribute value
            keys = paramKey.split("@")
            parameterKey = keys[0]
            attributeKey = keys[1]
            
            print "keys", keys
            self.serverClassData[serverId, classId].setAttribute(parameterKey, attributeKey, value)
            #if not self.__hash.has(paramKey):
            #    self.__hash.set(paramKey, None)
            #self.__hash.setAttribute(paramKey, attribute, value)
        else:
            self.serverClassData[serverId, classId].set(paramKey, value)
    
    
    def _changeDeviceData(self, key, value):
        deviceIdParamKey = key.split(".")
        if len(deviceIdParamKey) < 2:
            return

        deviceId = deviceIdParamKey[0]
        paramKey = deviceIdParamKey[1]
        
        if "@" in paramKey:
            # Merge attribute value
            keys = key.split("@")
            parameterKey = keys[0]
            attributeKey = keys[1]
            
            print "keys", keys
            self.deviceData[deviceId].setAttribute(parameterKey, attributeKey, value)
            #if not self.__hash.has(paramKey):
            #    self.__hash.set(paramKey, None)
            #self.__hash.setAttribute(paramKey, attribute, value)
        else:
            self.deviceData[deviceId].set(paramKey, value)


    #def _setFromPath(self, key, value):
        # pass key and value as list (immutable, important for ungoing procedures)
    #    print "key:", key, "value:", value
        # Safety conversion before hashing
    #    if "@" in key:
            # Merge attribute value in central hash
    #        keys = key.split("@")
    #        paramKey = keys[0]
    #        attribute = keys[1]
            
    #        if not self.__hash.has(paramKey):
    #            self.__hash.set(paramKey, 0)
    #        self.__hash.setAttribute(paramKey, attribute, value)
    #    else:
            # Merge parameter value in central hash
            #self.__hash.set(key, value)


    def disconnectedFromServer(self):
        # Reset manager settings
        self.reset()
        # Inform others about changes
        self.handleSystemTopology(Hash())
        # Send reset signal to configurator to clear stacked widget
        self.signalReset.emit()


    def registerEditableComponent(self, key, component):
        dataNotifier = self._getDataNotifierEditableValue(key)
        if dataNotifier is None:
            self.__keyNotifierMapEditableValue[key] = DataNotifier(key, component)
        else:
            dataNotifier.addComponent(key, component)


    def unregisterEditableComponent(self, key, component):
        pass


    def registerDisplayComponent(self, key, component):
        dataNotifier = self._getDataNotifierDisplayValue(key)
        if dataNotifier is None:
            self.__keyNotifierMapDisplayValue[key] = DataNotifier(key, component)
        else:
            dataNotifier.addComponent(key, component)


    def unregisterDisplayComponent(self, key, component):
        pass


    def registerHistoricData(self, key, slot):
        dataNotifier = self._getDataNotifierDisplayValue(key)
        dataNotifier.signalHistoricData.connect(slot)


    def handleHistoricData(self, headerHash, bodyHash):
        key = "{}.{}".format(headerHash.get("deviceId"), headerHash.get("property"))
        dataNotifier = self._getDataNotifierDisplayValue(key)
        dataNotifier.signalHistoricData.emit(key, bodyHash.get("data"))


    def _getDeviceIdFromInternalPath(self, internalPath):
        """
        This function gets as parameter the \internalPath of a device or a device
        class.
        Only if this internal path belongs to a device, the deviceId is return,
        otherwise return None.
        """
        print "_getDeviceIdFromInternalPath", internalPath
        splittedPath = internalPath.split('.')
        if len(splittedPath) < 1:
            # No device selected
            return None

        print "--- ", splittedPath
        return splittedPath[0]


    def newVisibleDevice(self, deviceId):
        # Check, whether deviceId is in systemTopology (means online)
        hasDevice = self.systemTopology.has(deviceId)
        # If schema was not seen, request device schema in function
        # getDeviceSchema will call newVisibleDevice again
        if hasDevice and (self.getDeviceSchema(deviceId) is None):
            return True

        deviceIdCount = self.__visibleDevInsKeys.get(deviceId)
        if deviceIdCount:
            self.__visibleDevInsKeys[deviceId] += 1
        else:
            self.__visibleDevInsKeys[deviceId] = 1
        if self.__visibleDevInsKeys[deviceId] == 1:
            self.signalNewVisibleDevice.emit(deviceId)
        
        print "newVisible", self.__visibleDevInsKeys
        print ""
        return True


    def removeVisibleDevice(self, deviceId):
        deviceIdCount = self.__visibleDevInsKeys.get(deviceId)
        if deviceIdCount:
            self.__visibleDevInsKeys[deviceId] -= 1
            if self.__visibleDevInsKeys[deviceId] == 0:
                self.signalRemoveVisibleDevice.emit(deviceId)


    def _changeHash(self, devicePath, config, configChangeType=ConfigChangeTypes.DEVICE_INSTANCE_CURRENT_VALUES_CHANGED):
        # Go recursively through Hash
        self._r_checkHash(devicePath, devicePath, config, configChangeType)


    def _r_checkHash(self, path, devicePath, config, configChangeType):
        topLevelKeys = config.keys()
        for key in topLevelKeys:
            value = config.get(key)
            try:
                timestamp = Timestamp.fromHashAttributes(
                    config.getAttributes(key))
            except RuntimeError as e:
                timestamp = None

            if len(path) < 1:
                internalPath = key
            else:
                internalPath = path + "." + key
            
            #self._setFromPath(internalPath, value)
            
            # Check if DataNotifier for key available
            if configChangeType is ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED:
                dataNotifier = self._getDataNotifierEditableValue(internalPath)
                if dataNotifier is not None:
                    dataNotifier.signalUpdateComponent.emit(internalPath, value, timestamp)
            elif configChangeType is ConfigChangeTypes.DEVICE_INSTANCE_CONFIG_CHANGED:
                dataNotifier = self._getDataNotifierEditableValue(internalPath)
                if dataNotifier is not None:
                    dataNotifier.signalUpdateComponent.emit(internalPath, value, timestamp)
            elif configChangeType is ConfigChangeTypes.DEVICE_INSTANCE_CURRENT_VALUES_CHANGED:
                dataNotifier = self._getDataNotifierDisplayValue(internalPath)
                if dataNotifier is not None:
                    dataNotifier.signalUpdateComponent.emit(internalPath, value, timestamp)
                
                # Notify editable widget of display value change
                dataNotifier = self._getDataNotifierEditableValue(internalPath)
                if dataNotifier is not None:
                    # Broadcast new displayValue to all editable widgets
                    dataNotifier.updateDisplayValue(internalPath, value, timestamp)
                
                # Check state
                if key == "state":
                    self._triggerStateChange(devicePath, value)
                        
            # More recursion in case of Hash type
            if isinstance(value, Hash):
                self._r_checkHash(internalPath, devicePath, value, configChangeType)
            elif isinstance(value, list):
                # TODO: needs to be implemented
                for i in xrange(len(value)):
                    internalPath = internalPath + "[" + str(i) + "]"
                    hashValue = value[i]


    def _triggerStateChange(self, devicePath, value):
        deviceId = devicePath.split('.configuration',1)[0]
        # Update GUI due to state changes
        if value == "Changing...":
            self.signalChangingState.emit(deviceId, True)
        else:
            if ("Error" in value) or ("error" in value):
                self.signalErrorState.emit(deviceId, True)
            else:
                self.signalErrorState.emit(deviceId, False)
            
            self.signalChangingState.emit(deviceId, False)
            self.signalDeviceStateChanged.emit(deviceId, value)


### Slots ###
    def onDeviceClassValueChanged(self, key, value):
        self._changeClassData(key, value)

        dataNotifier = self._getDataNotifierEditableValue(key)
        if dataNotifier is not None:
            dataNotifier.signalUpdateComponent.emit(key, value, None)


    def onDeviceInstanceValueChanged(self, key, value):
        self._changeDeviceData(key, value)

        dataNotifier = self._getDataNotifierEditableValue(key)
        if dataNotifier is not None:
            dataNotifier.signalUpdateComponent.emit(key, value, None)
        
        keys = key.split(".")
        deviceId = keys[0]
        parameterKey = keys[1]

        # Informs network
        self.signalReconfigure.emit(deviceId, parameterKey, value)


    def onDeviceChangedAsHash(self, instanceKey, config):
        paths = config.paths()
        for path in paths:
            dataNotifier = self._getDataNotifierEditableValue(path)
            if dataNotifier is not None:
                dataNotifier.signalUpdateComponent.emit(path, config.get(path),
                                                        None)
        
        self._mergeIntoHash(config)

        keys = str(instanceKey).split('.')
        instanceId = keys[1]
        self.signalReconfigureAsHash.emit(instanceId, config.get(instanceKey + ".configuration"))


    def onConflictStateChanged(self, key, hasConflict):
        self.signalConflictStateChanged.emit(key, hasConflict)


    def initDevice(self, serverId, classId):
        # Put configuration hash together
        config = Hash(classId, self.serverClassData[serverId, classId].configuration)
        
        # Send signal to network
        self.signalInitDevice.emit(serverId, config)
        self.__isInitDeviceCurrentlyProcessed = True


    def killDevice(self, deviceId):
        reply = QMessageBox.question(None, 'Message',
            "Do you really want to kill this instance?", QMessageBox.Yes |
            QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return

        # Remove deviceId data
        del self.deviceData[deviceId]
        self.signalKillDevice.emit(deviceId)


    def killServer(self, serverId):
        reply = QMessageBox.question(None, 'Message',
            "Do you really want to kill this instance?", QMessageBox.Yes |
            QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return
        
        # Remove device instance data from internal hash
        #self._setFromPath("server." + serverId, Hash())
        self.signalKillServer.emit(serverId)


### TODO: Temporary functions for scientific computing START ###
    def createNewProjectConfig(self, customItem, path, configCount, classId, schema):
        
        configName = "{}-{}-<>".format(configCount, classId)
        
        self.signalCreateNewProjectConfig.emit(customItem, path, configName)
        self.signalSchemaAvailable.emit(dict(key=path, schema=schema, classId=classId, type=NavigationItemTypes.CLASS))
        self.signalProjectItemChanged.emit(dict(key=path))
        
        #print self.__projectHash
        #print "-----"


    def createNewConfigKeyAndCount(self, classId):
        nbConfigs = len(self.__projectArrayIndices)
        
        path = "project.devices.[" + str(nbConfigs) + "]." + str(classId)
        self.__projectHash.set(path, Hash())
        
        self.__projectArrayIndices.append(nbConfigs+1)
        
        return (path, nbConfigs)

    # project hash:
    # project name="test" +
    #   devices +
    #     0 +
    #       classId1 +
    #         deviceId = s1
    #     1 +
    #       classId2 +

### TODO: Temporary functions for scientific computing END ###


    def executeCommand(self, itemInfo):
        # instanceId, name, arguments
        path = itemInfo.get('path')
        keys = str(path).split('.')
        deviceId = keys[1]
        
        command = itemInfo.get('command')
        args = itemInfo.get('args')
        
        self.signalExecute.emit(deviceId, dict(command=str(command), args=args))


    def onLogDataAvailable(self, logData):
        # Send message to logging panel
        self.signalLogDataAvailable.emit(logData)


    def onRefreshInstance(self, internalPath):
        deviceId = self._getDeviceIdFromInternalPath(internalPath)
        if (not deviceId) and (not self.__hash.has(internalPath)):
            return
        self.signalRefreshInstance.emit(deviceId)

   
    def onNewNavigationItem(self, itemInfo):
        self.signalNewNavigationItem.emit(itemInfo)


    def selectDeviceByPath(self, path):
        if self.__isInitDeviceCurrentlyProcessed is True:
            self.signalSelectNewNavigationItem.emit(path)
            self.__isInitDeviceCurrentlyProcessed = False


    def potentiallyRefreshVisibleDevice(self, deviceId):
        # If deviceId is already visible in scene but was offline, force refresh
        if (deviceId in self.__visibleDevInsKeys) and \
           (self.__visibleDevInsKeys[deviceId] > 0):
            self.signalSelectNewNavigationItem.emit(deviceId)
            self.signalRefreshInstance.emit(deviceId)


    def onSchemaAvailable(self, itemInfo):
        # Notify ConfigurationPanel
        self.signalSchemaAvailable.emit(itemInfo)


    def onDeviceInstanceChanged(self, itemInfo, xml):
        self.signalDeviceInstanceChanged.emit(itemInfo, xml)


    def openAsXml(self, filename, path, configChangeType, classId):
        config = loadFromFile(str(filename))
        
        tmp = config.get(classId)
        
        # TODO: not working correctly yet
        # Validate against fullSchema - state dependent configurations
        #fullSchema = Schema()
        #rules = None
        
        # Get full schema of device class
        #path = internalKey + ".description"
        #if self.__hash.has(path):
        #    fullSchema = self.__hash.get(path)
            
            # Get current state
        #    state = None
        #    statePath = internalKey + ".configuration.state"
        #    if self.__hash.has(statePath):
        #        state = self.__hash.get(statePath)
            
        #    splitPath = path.split(".")
        #    if splitPath[0] == "device":
        #        print "device"
        #        rules = AssemblyRules(AccessType.WRITE, state)#accessMode, state, globals.GLOBAL_ACCESS_LEVEL)
        #    else:
        #        print "class"
        #        rules = AssemblyRules(AccessType.INIT)#accessMode, state, globals.GLOBAL_ACCESS_LEVEL)

        #schema = Schema(classId, rules)
        #schema.merge(fullSchema)
        
        #validationRules = ValidatorValidationRules()
        #validationRules.allowAdditionalKeys        = True
        #validationRules.allowMissingKeys           = True
        #validationRules.allowUnrootedConfiguration = True
        #validationRules.injectDefaults             = False
        #validationRules.injectTimestamps           = False
        #validator = Validator()
        #validator.setValidationRules(validationRules)
        # Get validated config
        #config = validator.validate(schema, config)

        # TODO: Reload XSD in configuration panel
        # ...
        
        # Remove old data from internal hash
        self._setFromPath(path, Hash())
        
        # Update internal hash with new data for path
        self._changeHash(path, tmp, configChangeType)
        self._mergeIntoHash(Hash(path, tmp))


    def onFileOpen(self, configChangeType, path, classId=str()):
        filename = QFileDialog.getOpenFileName(None, "Open saved configuration", QDir.tempPath(), "XML (*.xml)")
        if len(filename) < 1:
            return
        
        file = QFile(filename)
        if file.open(QIODevice.ReadOnly | QIODevice.Text) == False:
            return
        
        self.openAsXml(filename, path, configChangeType, classId)


    def saveAsXml(self, filename, classId, path):
        if self.__hash.has(path):
            config = Hash(classId, self.__hash.get(path))
        else:
            config = Hash()

        saveToFile(config, filename)

    
    def onSaveAsXml(self, classId, path):
        filename = QFileDialog.getSaveFileName(None, "Save file as", QDir.tempPath(), "XML (*.xml)")
        if len(filename) < 1:
            return
        
        fi = QFileInfo(filename)
        if len(fi.suffix()) < 1:
            filename += ".xml"

        self.saveAsXml(str(filename), classId, path)


    # TODO: needs to be implemented
    def onReloadXsd(self, deviceServer, deviceId):
        print "Manager, onReloadXsd", deviceServer, deviceId
        #mainWindow signalReloadXsd
        # onXsdAvailable.emit()
        pass


    def handleSystemTopology(self, config):
        # Update navigation treemodel
        self.systemTopology.updateData(config)


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
        instanceIds = self.removeExistingInstances(config)
        for id in instanceIds:
            timestamp = datetime.datetime.now()
            # TODO: better format for timestamp and timestamp generation in karabo
            timestamp = timestamp.strftime("%Y-%m-%d %H:%M:%S")
            logMessage = timestamp + " | " + "INFO" + " | " + id + " | " \
                         "Detected dirty shutdown for instance \"" + id + "\", which " \
                         "is coming up now.#"
            # A log message is triggered
            self.onLogDataAvailable(logMessage)

        # Update system topology with new configuration
        self.systemTopology.instanceNew(config)

        # If device was instantiated from GUI, it should be selected after coming up
        deviceKey = "device"
        if config.has(deviceKey):
            deviceConfig = config.get(deviceKey)
            deviceIds = list()
            deviceConfig.getKeys(deviceIds)
            for deviceId in deviceIds:
                self.selectDeviceByPath(deviceId)
                self.potentiallyRefreshVisibleDevice(deviceId)


    def handleInstanceUpdated(self, config):
        self.systemTopology.instanceUpdated(config)


    def handleInstanceGone(self, instanceId):
        """
        Remove instanceId from central hash and update
        """
        path = None
        fullServerPath = "server.{}".format(instanceId)
        fullDevicePath = "device.{}".format(instanceId)
        if self.systemTopology.currentConfig.has(fullServerPath):
            if self.systemTopology.currentConfig.hasAttribute(fullServerPath, "host"):
                parentPath = self.systemTopology.currentConfig.getAttribute(fullServerPath, "host")
            path = fullServerPath
        elif self.systemTopology.currentConfig.has(fullDevicePath):
            if self.systemTopology.currentConfig.hasAttribute(fullDevicePath, "serverId"):
                parentPath = self.systemTopology.currentConfig.getAttribute(fullDevicePath, "serverId")
            if self.systemTopology.currentConfig.hasAttribute(fullDevicePath, "classId"):
                parentPath += ".{}".format(self.systemTopology.currentConfig.getAttribute(fullDevicePath, "classId"))
            path = fullDevicePath
                
        if path is None:
            print "Unknown instance \"" + instanceId + "\" gone."
            return

        # Update system topology
        self.systemTopology.instanceGone(path)
        self.signalInstanceGone.emit(instanceId, parentPath)


    def handleClassSchema(self, headerHash, config):
        serverId = headerHash.get("serverId")
        classId = headerHash.get("classId")
        schema = config.get("schema")
        
        # Update map for server and class with schema
        self.serverClassData[serverId, classId] = Configuration(schema)
        path = "{}.{}".format(serverId, classId)
        self.onSchemaAvailable(dict(key=path, classId=classId, 
                               type=NavigationItemTypes.CLASS, schema=schema))


    def getClassSchema(self, serverId, classId):
        # Return class schema, if already existing
        if (serverId, classId) in self.serverClassData:
            return self.serverClassData[serverId, classId].schema

        # Else, send network request
        self.signalGetClassSchema.emit(serverId, classId)
        return None
    
    
    def handleDeviceSchema(self, headerHash, config):
        deviceId = headerHash.get("deviceId")
        if deviceId in self.deviceData:
            return
        
        # Add configuration with schema to device data
        schema = config.get("schema")
        self.deviceData[deviceId] = Configuration(schema)
        
        self.onSchemaAvailable(dict(key=deviceId, type=NavigationItemTypes.DEVICE,
                                    schema=schema))
        self.newVisibleDevice(deviceId)


    def getDeviceSchema(self, deviceId):
        if deviceId in self.deviceData:
            return self.deviceData[deviceId].schema
        
        # Send network request
        self.signalGetDeviceSchema.emit(deviceId)
        return None
        

    def handleDeviceSchemaUpdated(self, headerHash, config):
        path = "device." + headerHash.get("deviceId")
        self.signalDeviceSchemaUpdated.emit(path)
        self.handleDeviceSchema(headerHash, config)
        self.onRefreshInstance(path)


    # TODO: This function must be thread-safe!!
    def handleConfigurationChanged(self, headerHash, config):
        deviceId = headerHash.get("deviceId")
        self._changeHash(deviceId, config)
        # Merge configuration into self.deviceData
        self.deviceData[deviceId].merge(config)


    def handleNotification(self, timestamp, type, shortMessage, detailedMessage, deviceId):
        self.signalNotificationAvailable.emit(timestamp, type, shortMessage, detailedMessage, deviceId)


    def removeExistingInstances(self, config):
        """
        This function checks whether instances already exist in the central
        hash.
        if \True, these instance is erased from the central hash
        if \False, nothing happens
        A list with removed instances is returned.
        """
        removedInstanceIds = []
        serverKey = "server"

        # Check servers
        if config.has(serverKey):
            serverIds = config.get(serverKey).keys()
            for serverId in serverIds:
                # Check, if serverId is already in central hash
                path = serverKey + "." + serverId
                if self.systemTopology.currentConfig.has(path):
                    # Check old classes and send signal to remove old configuration pages
                    serverConfig = self.systemTopology.currentConfig.get(serverKey)
                    if serverConfig.hasAttribute(serverId, "deviceClasses"):
                        classes = serverConfig.getAttribute(serverId, "deviceClasses")
                        visibilities = serverConfig.getAttribute(serverId, "visibilities")
                        i = -1
                        for classId in classes:
                            i = i + 1
                            if visibilities[i] <= globals.GLOBAL_ACCESS_LEVEL:
                                classPath = "{}.{}".format(serverId, classId)
                                # Remove configuration page for associated class
                                self.signalInstanceNewReset.emit(classPath)
                    # Remove path from central hash
                    self.systemTopology.currentConfig.erase(path)
                    removedInstanceIds.append(serverId)
                    # Check for running instances on server
                    self._removeExistingDevices(self.systemTopology.currentConfig, removedInstanceIds, serverId)

        # Check devices
        self._removeExistingDevices(config, removedInstanceIds)
        
        return removedInstanceIds


    def _removeExistingDevices(self, config, removedInstanceIds, serverId=None):
        """
        This function checks whether device instances or device instances with"
        "\serverId already exist in the central hash.
        if \True, these instance is erased from the central hash
        if \False, nothing happens
        
        A list with removed instances is filled.
        """
        deviceKey = "device"

        if config.has(deviceKey):
            deviceIds = config.get(deviceKey).keys()
            for deviceId in deviceIds:
                path = deviceKey + "." + deviceId
                if serverId:
                    # Check, if there is a device running on server
                    if config.hasAttribute(path, "serverId"):
                        id = config.getAttribute(path, "serverId")
                        if serverId == id:
                            config.erase(path)
                            removedInstanceIds.append(deviceId)
                else:
                    # Check, if deviceId is already in central hash
                    if self.systemTopology.currentConfig.has(path):
                        # Remove path from central hash
                        self.systemTopology.currentConfig.erase(path)
                        removedInstanceIds.append(deviceId)


manager = _Manager()

def Manager():
    return manager
