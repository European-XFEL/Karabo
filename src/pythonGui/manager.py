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
from datetime import datetime
from enums import NavigationItemTypes
from enums import ConfigChangeTypes
from karabo.karathon import (Hash, loadFromFile, saveToFile, Timestamp)
from navigationtreemodel import NavigationTreeModel
from projectmodel import ProjectModel
from sqldatabase import SqlDatabase

from PyQt4.QtCore import (pyqtSignal, QDir, QFile, QFileInfo, QIODevice, QObject)
from PyQt4.QtGui import (QFileDialog, QMessageBox)

useOldVersion = True

class DataNotifier(QObject):
    signalUpdateComponent = pyqtSignal(str, object, object) # internalKey, value, timestamp
    signalUpdateDisplayValue = pyqtSignal(str, object, object)
    signalHistoricData = pyqtSignal(str, object)


    def __init__(self, key, component):
        super(DataNotifier, self).__init__()

        if useOldVersion:
            self.components = [] # list of components
        else:
            self.signalUpdateComponent.connect(self.onValueChanged)
            self.signalUpdateDisplayValue.connect(self.onValueChanged)
        self.addComponent(key, component)


    def onValueChanged(self, key, value, timestamp=None):
        self.value = value
        self.timestamp = timestamp


    def addComponent(self, key, component):
        self.signalUpdateComponent.connect(component.onValueChanged)
        self.signalUpdateDisplayValue.connect(component.onDisplayValueChanged)
        
        if useOldVersion:
            if len(self.components) > 0:
                value = self.components[0].value
                self.signalUpdateComponent.emit(key, value)
                self.signalUpdateDisplayValue.emit(key, value)
                
                # Add widget to list
                self.components.append(component)
        else:
            if hasattr(self, "value"):
                self.signalUpdateComponent.emit(key, self.value, self.timestamp)
                self.signalUpdateDisplayValue.emit(key, self.value, self.timestamp)


    def updateDisplayValue(self, key, value, timestamp):
        self.signalUpdateDisplayValue.emit(key, value, timestamp)


class _Manager(QObject):
    # signals
    signalGlobalAccessLevelChanged = pyqtSignal()

    signalReset = pyqtSignal()

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

    signalReconfigure = pyqtSignal(str, str, object) # deviceId, property, value
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

    signalGetClassSchema = pyqtSignal(str, str) # serverId, classId
    signalGetDeviceSchema = pyqtSignal(str) # deviceId
    signalGetFromPast = pyqtSignal(str, str, str, str) # deviceId, property, t0, t1


    def __init__(self, *args, **kwargs):
        super(_Manager, self).__init__()

        # Map stores all keys and DataNofiers for editable widgets
        self.__keyNotifierMapEditableValue = dict()
        # Map stores all keys and DataNofiers for display widgets
        self.__keyNotifierMapDisplayValue = dict()
        
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
        
        
        # Map stores { (serverId, class), Configuration }
        self.serverClassData = dict()
        # Map stores { deviceId, Configuration }
        self.deviceData = dict()
        
        # Sets all parameters to start configuration
        self.reset()


    # Sets all parameters to start configuration
    def reset(self):
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
            
            self.serverClassData[serverId, classId].setAttribute(parameterKey, attributeKey, value)
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
            
            self.deviceData[deviceId].setAttribute(parameterKey, attributeKey, value)
        else:
            self.deviceData[deviceId].set(paramKey, value)


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


    def handleHistoricData(self, instanceInfo):
        key = "{}.{}".format(instanceInfo.get("deviceId"), instanceInfo.get("property"))
        dataNotifier = self._getDataNotifierDisplayValue(key)
        dataNotifier.signalHistoricData.emit(key, instanceInfo.get("data"))


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


    def newVisibleDevice(self, internalPath):
        """
        This function registers a new visible instance in a map. If it is the
        first occurence, a signal is sent to the network to inform the GuiServerDevice.
        
        The incoming parameter \internalPath can be either directly a deviceId
        or an internalPath of an property item which needs to be splitted to get
        the deviceId.
        """
        deviceId = self._getDeviceIdFromInternalPath(internalPath)
        if deviceId is None:
            return
        
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
                print "removeVisibleDevice", deviceId


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
        print "onDeviceClassValueChanged", key, value
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
        property = keys[1]

        # Informs network
        self.signalReconfigure.emit(deviceId, property, value)


    def onDeviceChangedAsHash(self, deviceId, config):
        paths = config.paths()
        for path in paths:
            dataNotifier = self._getDataNotifierEditableValue(path)
            if dataNotifier is not None:
                dataNotifier.signalUpdateComponent.emit(path, config.get(path),
                                                        None)
        config = Hash(config.get(deviceId))
        self.deviceData[deviceId].configuration = config
        self.signalReconfigureAsHash.emit(deviceId, config)


    def onConflictStateChanged(self, key, hasConflict):
        self.signalConflictStateChanged.emit(key, hasConflict)


    def onNavigationTreeModelSelectionChanged(self, selected, deselect):
        """
        This slot is called whenever something of the navigation panel is selected.
        If an item was selected, the selection of the project panel is cleared.
        """
        if len(selected.indexes()) < 1:
            return

        self.projectTopology.selectionModel.clearSelection()


    def onProjectModelSelectionChanged(self, selected, deselected):
        """
        This slot is called whenever something of the project panel is selected.
        If an item was selected, the selection of the navigation panel is cleared.
        """
        if len(selected.indexes()) < 1:
            return

        self.systemTopology.selectionModel.clearSelection()



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


    def onRefreshInstance(self, internalPath):
        deviceId = self._getDeviceIdFromInternalPath(internalPath)
        if deviceId is None:
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


    def openAsXml(self, filename, deviceId, classId, serverId):
        config = loadFromFile(str(filename))
        # Needs to be copied into new Hash to prevent segmentation fault
        tmp = Hash(config.get(classId))
        
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
        
        changeType = None
        if deviceId is not None:
            path = deviceId
            changeType = ConfigChangeTypes.DEVICE_INSTANCE_CONFIG_CHANGED
            # Merge new config into internal datastructure
            self.deviceData[deviceId].configuration = tmp
        elif serverId is not None:
            path = "{}.{}".format(serverId, classId)
            changeType = ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED
            # Merge new config into internal datastructure
            self.serverClassData[serverId, classId].configuration = tmp
        
        # Update view with new data for path
        self._changeHash(path, tmp, changeType)


    def onFileOpen(self, deviceId, classId, serverId):
        filename = QFileDialog.getOpenFileName(None, "Open saved configuration", \
                                               QDir.tempPath(), "XML (*.xml)")
        if len(filename) < 1:
            return
        
        file = QFile(filename)
        if file.open(QIODevice.ReadOnly | QIODevice.Text) == False:
            return
            
        self.openAsXml(filename, deviceId, classId, serverId)


    def saveAsXml(self, filename, deviceId, classId, serverId=None):
        if deviceId is not None:
            config = Hash(classId, self.deviceData[deviceId].configuration)
        elif serverId is not None:
            config = Hash(classId, self.serverClassData[serverId, classId].configuration)
        else:
            config = Hash()

        saveToFile(config, filename)

    
    def onSaveAsXml(self, deviceId, classId, serverId):
        filename = QFileDialog.getSaveFileName(None, "Save file as", QDir.tempPath(), "XML (*.xml)")
        if len(filename) < 1:
            return
        
        fi = QFileInfo(filename)
        if len(fi.suffix()) < 1:
            filename += ".xml"

        self.saveAsXml(filename, deviceId, classId, serverId)


    # TODO: needs to be implemented
    def onReloadXsd(self, deviceServer, deviceId):
        print "Manager, onReloadXsd", deviceServer, deviceId
        #mainWindow signalReloadXsd
        # onXsdAvailable.emit()
        pass


    def handleSystemTopology(self, config):
        # Update navigation treemodel
        self.systemTopology.updateData(config)
        # Update project model
        self.projectTopology.systemTopologyChanged(config)


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
        # Update system topology
        parentPath = self.systemTopology.erase(instanceId)
        if parentPath is not None:
            self.signalInstanceGone.emit(instanceId, parentPath)
        
        # Update on/offline status of project
        self.projectTopology.handleInstanceGone(instanceId)


    def handleClassSchema(self, classInfo):
        serverId = classInfo.get('serverId')
        classId = classInfo.get('classId')
        schema = classInfo.get('schema')
        
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
    
    
    def handleDeviceSchema(self, instanceInfo):
        deviceId = instanceInfo.get('deviceId')
        if (deviceId in self.deviceData) and (self.deviceData[deviceId].schema is not None):
            return
        
        # Add configuration with schema to device data
        schema = instanceInfo.get('schema')
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
        

    def handleDeviceSchemaUpdated(self, instanceInfo):
        deviceId = instanceInfo.get("deviceId")
        if deviceId in self.deviceData:
            self.deviceData[deviceId].schema = None
        
        self.signalDeviceSchemaUpdated.emit(deviceId)
        self.handleDeviceSchema(instanceInfo)
        self.onRefreshInstance(deviceId)


    # TODO: This function must be thread-safe!!
    def handleConfigurationChanged(self, instanceInfo):
        deviceId = instanceInfo.get("deviceId")
        config = instanceInfo.get("configuration")
        self._changeHash(deviceId, config)
        # Merge configuration into self.deviceData
        self.deviceData[deviceId].merge(config)


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
