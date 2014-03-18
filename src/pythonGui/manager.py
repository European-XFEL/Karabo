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


from enums import NavigationItemTypes, ConfigChangeTypes
from configuration import Configuration
import datetime
from hash import Hash, HashMergePolicy
from timestamp import Timestamp
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

    signalNewNavigationItem = pyqtSignal(dict) # id, name, type, (status), (refType), (refId), (schema)
    signalSelectNewNavigationItem = pyqtSignal(str) # deviceId
    signalSchemaAvailable = pyqtSignal(object) # key, schema
    signalDeviceInstanceChanged = pyqtSignal(dict, str)
    signalKillDevice = pyqtSignal(str) # deviceId
    signalKillServer = pyqtSignal(str) # serverId
    signalDeviceSchemaUpdated = pyqtSignal(str) # deviceId

    signalRefreshInstance = pyqtSignal(str) # deviceId
    signalInitDevice = pyqtSignal(str, object) # deviceId, hash
    signalExecute = pyqtSignal(str, dict) # deviceId, slotName/arguments

    signalReconfigure = pyqtSignal(list)
    signalDeviceStateChanged = pyqtSignal(object, str) # fullDeviceKey, state
    signalConflictStateChanged = pyqtSignal(object, bool) # key, hasConflict
    signalChangingState = pyqtSignal(object, bool) # deviceId, isChanging
    signalErrorState = pyqtSignal(object, bool) # deviceId, inErrorState

    signalInstanceGone = pyqtSignal(str, str) # path, parentPath

    signalNewVisibleDevice = pyqtSignal(object) # device
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
        
        # Map stores { (serverId, class), Configuration }
        self.serverClassData = dict()
        # Map stores { deviceId, Configuration }
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
    
    
    def disconnectedFromServer(self):
        # Reset manager settings
        self.reset()
        # Inform others about changes
        self.handleSystemTopology(Hash())
        # Send reset signal to configurator to clear stacked widget
        self.signalReset.emit()


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


    def _changeHash(self, devicePath, config, configChangeType=ConfigChangeTypes.DEVICE_INSTANCE_CURRENT_VALUES_CHANGED):
        # Go recursively through Hash
        self._r_changeHash(devicePath, devicePath, config, configChangeType)


    def _r_changeHash(self, path, devicePath, config, configChangeType):
        for key, value in config.iteritems():
            try:
                timestamp = Timestamp.fromHashAttributes(
                    config.getAttributes(key))
            except KeyError as e:
                timestamp = None

            if len(path) < 1:
                internalPath = key
            else:
                internalPath = path + "." + key

            # Check if DataNotifier for key available
            if configChangeType == ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED:
                dataNotifier = self._getDataNotifierEditableValue(internalPath)
                if dataNotifier is not None:
                    dataNotifier.signalUpdateComponent.emit(internalPath, value, timestamp)
            elif configChangeType == ConfigChangeTypes.DEVICE_INSTANCE_CONFIG_CHANGED:
                dataNotifier = self._getDataNotifierEditableValue(internalPath)
                if dataNotifier is not None:
                    dataNotifier.signalUpdateComponent.emit(internalPath, value, timestamp)
            elif configChangeType == ConfigChangeTypes.DEVICE_INSTANCE_CURRENT_VALUES_CHANGED:
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
                self._r_changeHash(internalPath, devicePath, value, configChangeType)
            elif isinstance(value, list):
                # TODO: needs to be implemented
                for i in xrange(len(value)):
                    internalPath = internalPath + "[" + str(i) + "]"
                    hashValue = value[i]


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
            self.signalDeviceStateChanged.emit(configuration, value)


### Slots ###
    def onDeviceClassValueChanged(self, key, value):
        self._changeClassData(key, value)

        dataNotifier = self._getDataNotifierEditableValue(key)
        if dataNotifier is not None:
            dataNotifier.signalUpdateComponent.emit(key, value, None)


    def onDeviceInstanceValuesChanged(self, boxes):
        self.signalReconfigure.emit(boxes)



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
        keys = path.split('.')
        deviceId = keys[0]
        
        command = itemInfo.get('command')
        args = itemInfo.get('args')
        
        self.signalExecute.emit(deviceId, dict(command=command, args=args))


    def onLogDataAvailable(self, logData):
        # Send message to logging panel
        self.signalLogDataAvailable.emit(logData)


    def onRefreshInstance(self, internalPath):
        print "onRefreshInstance", internalPath
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
        print "instanceIds", instanceIds
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
        self.systemTopology.updateData(config)

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
        self.systemTopology.updateData(config)


    def handleInstanceGone(self, instanceId):
        """
        Remove instanceId from central hash and update
        """
        # Update system topology
        parentPath = self.systemTopology.erase(instanceId)
        if parentPath is None:
            return
        
        self.signalInstanceGone.emit(instanceId, parentPath)


    def handleClassSchema(self, headerHash, config):
        serverId = headerHash.get('serverId')
        classId = headerHash.get('classId')
        schema = config.get('schema')
        
        conf = self.serverClassData[serverId, classId]
        conf.setSchema(schema)
        self.onSchemaAvailable(conf)


    def getClass(self, serverId, classId):
        if deviceId not in self.deviceData:
            path = "{}.{}".format(serverId, classId)
            self.deviceData[deviceId] = Configuration(path, 'device')
            self.signalGetDeviceSchema.emit(deviceId)
        return self.deviceData[deviceId]
    
    
    def handleDeviceSchema(self, headerHash, config):
        deviceId = headerHash['deviceId']
        if deviceId not in self.deviceData:
            print 'not requested schema for device {} arrived'.format(deviceId)
            return
        
        # Add configuration with schema to device data
        schema = config['schema']
        conf = self.deviceData[deviceId]
        conf.setSchema(schema)
        conf.configuration.state.signalUpdateComponent.connect(
            self._triggerStateChange)
        
        self.onSchemaAvailable(conf)
        conf.addVisible()


    def getDevice(self, deviceId):
        if deviceId not in self.deviceData:
            self.deviceData[deviceId] = Configuration(deviceId, 'device')
            self.signalGetDeviceSchema.emit(deviceId)
        return self.deviceData[deviceId]
        

    def handleDeviceSchemaUpdated(self, headerHash, config):
        path = "device." + headerHash.get("deviceId")
        self.signalDeviceSchemaUpdated.emit(path)
        self.handleDeviceSchema(headerHash, config)
        self.onRefreshInstance(path)


    # TODO: This function must be thread-safe!!
    def handleConfigurationChanged(self, headerHash, config):
        deviceId = headerHash.get("deviceId")
        self.deviceData[deviceId].merge(config)


def handleNotification(timestamp, type, shortMessage, detailedMessage, deviceId):
    Manager().signalNotificationAvailable.emit(timestamp, type, shortMessage, detailedMessage, deviceId)


manager = _Manager()

def Manager():
    return manager
