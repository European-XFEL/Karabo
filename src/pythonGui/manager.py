#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains the manager class which works a man in the middle of the
   star structure. All relevant signals go over here.
   
   The manager class is a singleton and has a notifier object due to problems with
   the definition of signals inside of a singleton class.
   
   All relevant configuration data is stored in a member hash variable.
   
   This notifier class contains only the signals needed to spread to all relevant
   places.
"""

__all__ = ["Manager"]


from datanotifier import DataNotifier
from enums import NavigationItemTypes
from enums import ConfigChangeTypes
import globals
from karabo.karathon import (Hash, HashMergePolicy, loadFromFile, saveToFile,
                             Timestamp)
from navigationhierarchymodel import NavigationHierarchyModel
from sqldatabase import SqlDatabase

from PyQt4.QtCore import (pyqtSignal, QDir, QFile, QFileInfo, QIODevice, QObject,
                          QTime, QTimer)
from PyQt4.QtGui import (QFileDialog, QMessageBox)

import time


class _Manager(QObject):
    """Class for signals which can not be integrated in Manager class"""
    # signals
    signalSystemTopologyChanged = pyqtSignal(object)

    signalGlobalAccessLevelChanged = pyqtSignal()

    signalReset = pyqtSignal()
    signalInstanceNewReset = pyqtSignal(str) # path

    signalNewNavigationItem = pyqtSignal(dict) # id, name, type, (status), (refType), (refId), (schema)
    signalSelectNewNavigationItem = pyqtSignal(str) # deviceId
    signalSchemaAvailable = pyqtSignal(dict) # key, schema
    signalNavigationItemChanged = pyqtSignal(dict) # type, key
    signalNavigationItemSelectionChanged = pyqtSignal(str) # key
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

    def __init__(self, *args, **kwargs):
        super(_Manager, self).__init__()
        
        # Map stores all keys and DataNofiers for editable widgets
        self.__keyNotifierMapEditableValue = dict()
        # Map stores all keys and DataNofiers for display widgets
        self.__keyNotifierMapDisplayValue = dict()
        
        # Initiate database connection
        self.__sqlDatabase = SqlDatabase()
        self.__sqlDatabase.openConnection()
        
        self.__treemodel = NavigationHierarchyModel()
        
        # Sets all parameters to start configuration
        self.reset()


    # Sets all parameters to start configuration
    def reset(self):
        # Reset Central hash
        self.__hash = Hash()
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


    def _hash(self):
        return self.__hash
    hash = property(fget=_hash)


    @property
    def notifier(self):
        return self


    def _sqlDatabase(self):
        return self.__sqlDatabase
    sqlDatabase = property(fget=_sqlDatabase)


    def _treemodel(self):
        return self.__treemodel
    treemodel = property(fget=_treemodel)


    def closeDatabaseConnection(self):
        self.__sqlDatabase.closeConnection()

    
    def _getDataNotifierEditableValue(self, key):
        key = str(key)
        return self.__keyNotifierMapEditableValue.get(key)


    def _getDataNotifierDisplayValue(self, key):
        key = str(key)
        return self.__keyNotifierMapDisplayValue.get(key)

    
    def _mergeIntoHash(self, config):
        #print ""
        #print config
        #print ""
        self.__hash.merge(config, HashMergePolicy.MERGE_ATTRIBUTES) #REPLACE_ATTRIBUTES)
        #print self.__hash


    def _setFromPath(self, key, value):
        # pass key and value as list (immutable, important for ungoing procedures)
        #print "key:", key, "value:", value
        key = str(key)
        # Safety conversion before hashing
        if "@" in key:
            # Merge attribute value in central hash
            keys = key.split("@")
            paramKey = keys[0]
            attribute = keys[1]
            if not self.__hash.has(paramKey):
                self.__hash.set(paramKey, 0)
            self.__hash.setAttribute(paramKey, attribute, value)
        else:
            # Merge parameter value in central hash
            self.__hash.set(key, value)
        
        #print ""
        #print self.__hash
        #print ""


    def disconnectedFromServer(self):
        # Reset manager settings
        self.reset()
        # Inform others about changes
        self.handleSystemTopology(Hash())
        # Send reset signal to configurator to clear stacked widget
        self.signalReset.emit()


    def registerEditableComponent(self, key, component):
        key = str(key)
        dataNotifier = self._getDataNotifierEditableValue(key)
        if dataNotifier is None:
            self.__keyNotifierMapEditableValue[key] = DataNotifier(key, component)
        else:
            dataNotifier.addComponent(key, component)


    def unregisterEditableComponent(self, key, component):
        key = str(key)
        dataNotifier = self._getDataNotifierEditableValue(key)
        if dataNotifier:
            dataNotifier.removeComponent(key, component)


    def registerDisplayComponent(self, key, component):
        key = str(key)
        dataNotifier = self._getDataNotifierDisplayValue(key)
        if dataNotifier is None:
            self.__keyNotifierMapDisplayValue[key] = DataNotifier(key, component)
        else:
            dataNotifier.addComponent(key, component)


    def unregisterDisplayComponent(self, key, component):
        key = str(key)
        dataNotifier = self._getDataNotifierDisplayValue(key)
        if dataNotifier:
            dataNotifier.removeComponent(key, component)
        

    def _getDeviceIdFromInternalPath(self, internalPath):
        """
        This function gets as parameter the \internalPath of a device or a device
        class.
        Only if this internal path belongs to a device, the deviceId is return,
        otherwise return None.
        """
        splittedPath = internalPath.split('device.')
        if len(splittedPath) < 2:
            # no device selected
            return None

        # Get deviceId
        deviceParameter = splittedPath[1].split('.configuration.')
        if len(deviceParameter) < 1:
            return None

        return deviceParameter[0]


    def newVisibleDevice(self, internalPath):
        deviceId = self._getDeviceIdFromInternalPath(internalPath)
        if not deviceId:
            return

        # Check whether deviceId in central hash
        hasDevice = self.__hash.has("device." + deviceId)
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
        
        return hasDevice


    def removeVisibleDevice(self, internalPath):
        deviceId = self._getDeviceIdFromInternalPath(internalPath)
        if not deviceId:
            return

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

            if len(path) < 1:
                internalPath = key
            else:
                internalPath = path + "." + key
            
            #self._setFromPath(internalPath, value)
            
            # Check if DataNotifier for key available
            if configChangeType is ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED:
                dataNotifier = self._getDataNotifierEditableValue(internalPath)
                if dataNotifier is not None:
                    dataNotifier.signalUpdateComponent.emit(internalPath, value)
            elif configChangeType is ConfigChangeTypes.DEVICE_INSTANCE_CONFIG_CHANGED:
                dataNotifier = self._getDataNotifierEditableValue(internalPath)
                if dataNotifier is not None:
                    dataNotifier.signalUpdateComponent.emit(internalPath, value)
            elif configChangeType is ConfigChangeTypes.DEVICE_INSTANCE_CURRENT_VALUES_CHANGED:
                dataNotifier = self._getDataNotifierDisplayValue(internalPath)
                if dataNotifier is not None:
                    dataNotifier.signalUpdateComponent.emit(internalPath, value)
                
                # Notify editable widget of display value change
                dataNotifier = self._getDataNotifierEditableValue(internalPath)
                if dataNotifier is not None:
                    # Broadcast new displayValue to all editable widgets
                    dataNotifier.updateDisplayValue(internalPath, value)
                
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
        #print "onDeviceClassValueChanged", key, value
        self._setFromPath(key, value)

        dataNotifier = self._getDataNotifierEditableValue(key)
        if dataNotifier is not None:
            dataNotifier.signalUpdateComponent.emit(key, value)


    def onDeviceInstanceValueChanged(self, key, value):
        #print "onDeviceInstanceValueChanged", key, value
        # Safety conversion before hashing
        self._setFromPath(key, value)

        dataNotifier = self._getDataNotifierEditableValue(key)
        if dataNotifier is not None:
            dataNotifier.signalUpdateComponent.emit(key, value)
        
        keys = str(key).split('.configuration.')
        deviceId = keys[0].split('.')[1]
        parameterKey = keys[1]

        # Informs network
        self.signalReconfigure.emit(deviceId, parameterKey, value)


    def onDeviceChangedAsHash(self, instanceKey, config):
        paths = config.paths()
        for path in paths:
            dataNotifier = self._getDataNotifierEditableValue(path)
            if dataNotifier is not None:
                dataNotifier.signalUpdateComponent.emit(path, config.get(path))
        
        self._mergeIntoHash(config)

        keys = str(instanceKey).split('.')
        instanceId = keys[1]
        self.signalReconfigureAsHash.emit(instanceId, config.get(instanceKey + ".configuration"))


    def onConflictStateChanged(self, key, hasConflict):
        self.signalConflictStateChanged.emit(key, hasConflict)


    def initDevice(self, serverId, classId, path):
        #print "initDevice", internalKey
        #print self.__hash
        
        h = Hash()
        if self.__hash.has(path):
            h = self.__hash.get(path)
        
        # TODO: Remove dirty hack for scientific computing again!!!
        croppedDevClaId = classId.split("-")
        classId = croppedDevClaId[0]
        
        config = Hash(classId, h)
        
        self.signalInitDevice.emit(serverId, config)
        self.__isInitDeviceCurrentlyProcessed = True


    def killDevice(self, deviceId):
        reply = QMessageBox.question(None, 'Message',
            "Do you really want to kill this instance?", QMessageBox.Yes |
            QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return

        # Remove device instance data from internal hash
        self._setFromPath("device." + deviceId, Hash())
        self.signalKillDevice.emit(deviceId)


    def killServer(self, serverId):
        reply = QMessageBox.question(None, 'Message',
            "Do you really want to kill this instance?", QMessageBox.Yes |
            QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return
        
        # Remove device instance data from internal hash
        self._setFromPath("server." + serverId, Hash())
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


    def selectNavigationItemByKey(self, path):
        self.signalNavigationItemSelectionChanged.emit(path)


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
            self.signalSelectNewNavigationItem.emit("device." + deviceId)
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


    def onNavigationItemChanged(self, itemInfo):
        self.signalNavigationItemChanged.emit(itemInfo)


    # TODO: needs to be implemented
    def onReloadXsd(self, deviceServer, deviceId):
        print "Manager, onReloadXsd", deviceServer, deviceId
        #mainWindow signalReloadXsd
        # onXsdAvailable.emit()
        pass


### New pythonGui2 stuff ###
    def handleSystemTopology(self, config):
        #print "handleSystemTopology"
        #print config
        #print ""
        # Merge new configuration data into central hash
        self._mergeIntoHash(config)
        
        # Send full internal hash to navigation
        self.signalSystemTopologyChanged.emit(self.__hash)


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
            timestamp = Timestamp()
            # TODO: better format for timestamp and timestamp generation in karabo
            timestamp = timestamp.toFormattedString("%Y-%m-%d %H:%M:%S")
            logMessage = timestamp + " | " + "INFO" + " | " + id + " | " \
                         "Detected dirty shutdown for instance \"" + id + "\", which " \
                         "is coming up now.#"
            # A log message is triggered
            self.onLogDataAvailable(logMessage)

        # Update central hash with new configuration
        self.handleSystemTopology(config)

        # If device was instantiated from GUI, it should be selected after coming up
        deviceKey = "device"
        if config.has(deviceKey):
            deviceConfig = config.get(deviceKey)
            deviceIds = list()
            deviceConfig.getKeys(deviceIds)
            for deviceId in deviceIds:
                self.selectDeviceByPath(deviceKey + "." + deviceId)
                self.potentiallyRefreshVisibleDevice(deviceId)


    def handleInstanceGone(self, instanceId):
        # Remove instanceId from central hash and update
        
        path = None
        if self.__hash.has("server." + instanceId):
            path = "server." + instanceId
            if self.__hash.hasAttribute(path, "host"):
                parentPath = self.__hash.getAttribute(path, "host")
        elif self.__hash.has("device." + instanceId):
            path = "device." + instanceId
            if self.__hash.hasAttribute(path, "serverId"):
                parentPath = "server." + self.__hash.getAttribute(path, "serverId")
            if self.__hash.hasAttribute(path, "classId"):
                parentPath += ".classes." + self.__hash.getAttribute(path, "classId")
                
        if path is None:
            print "Unknown instance \"" + instanceId + "\" gone."
            return
        
        # Remove instance from central hash
        self.__hash.erase(path)
        
        # Send full internal hash to navigation
        self.signalSystemTopologyChanged.emit(self.__hash)
        # Update navigation and configuration panel
        self.signalInstanceGone.emit(path, parentPath)


    def handleClassSchema(self, config):
        path = str(config.paths()[0])
        schema = config.get(path)
        # Merge new configuration data into central hash
        self._mergeIntoHash(config)
        
        path = path.split('.description', 1)[0]
        classId = path.split('.')[3]
        self.onSchemaAvailable(dict(key=path, classId=classId, type=NavigationItemTypes.CLASS, schema=schema))


    def getClassSchema(self, serverId, classId):
        path = str("server." + serverId + ".classes." + classId + ".description")
        if self.__hash.has(path):
            return self.__hash.get(path)

        # Send network request
        self.signalGetClassSchema.emit(serverId, classId)
        return None
    
    
    def handleDeviceSchema(self, deviceId, config):
        # Merge new configuration data into central hash
        self._mergeIntoHash(config)

        path = "device." + deviceId
        descriptionPath = path + ".description"
        self.onSchemaAvailable(dict(key=path, type=NavigationItemTypes.DEVICE, schema=config.get(descriptionPath)))
        self.newVisibleDevice(path)


    def getDeviceSchema(self, deviceId):
        path = str("device." + deviceId + ".description")
        if self.__hash.has(path):
            return self.__hash.get(path)

        # Send network request
        self.signalGetDeviceSchema.emit(deviceId)
        return None
        

    def handleDeviceSchemaUpdated(self, deviceId, config):
        path = "device." + deviceId
        self.signalDeviceSchemaUpdated.emit(path)
        self.handleDeviceSchema(deviceId, config)
        # Refresh needed
        self.onRefreshInstance(path)


    # TODO: This function must be thread-safe!!
    # Called by network class
    def handleConfigurationChanged(self, deviceId, config):
        configurationPath = "device." + deviceId + ".configuration"
        self._changeHash(configurationPath, config.get(configurationPath))
        
        # Merge new configuration data into central hash
        self._mergeIntoHash(config)


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
                if self.__hash.has(path):
                    # Check old classes and send signal to remove old configuration pages
                    serverConfig = self.__hash.get(serverKey)
                    if serverConfig.hasAttribute(serverId, "deviceClasses"):
                        classes = serverConfig.getAttribute(serverId, "deviceClasses")
                        visibilities = serverConfig.getAttribute(serverId, "visibilities")
                        i = -1
                        for deviceClass in classes:
                            i = i + 1
                            if visibilities[i] <= globals.GLOBAL_ACCESS_LEVEL:
                                classPath = path + ".classes." + deviceClass
                                # Remove configuration page for associated class
                                self.signalInstanceNewReset.emit(classPath)
                    # Remove path from central hash
                    self.__hash.erase(path)
                    removedInstanceIds.append(serverId)
                    # Check for running instances on server
                    self._removeExistingDevices(self.__hash, removedInstanceIds, serverId)

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
                    if self.__hash.has(path):
                        # Remove path from central hash
                        self.__hash.erase(path)
                        removedInstanceIds.append(deviceId)


manager = _Manager()

def Manager():
    return manager
