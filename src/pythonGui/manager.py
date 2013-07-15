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

__all__ = ["Notifier", "Manager"]


from datanotifier import DataNotifier
from enums import NavigationItemTypes
from enums import ConfigChangeTypes
from karabo.karathon import *
from navigationhierarchymodel2 import NavigationHierarchyModel
from singleton import Singleton
from sqldatabase import SqlDatabase

from PyQt4.QtCore import *
from PyQt4.QtGui import *

import time


class Notifier(QObject):
    """Class for signals which can not be integrated in Manager class"""
    # signals
    signalSystemTopologyChanged = pyqtSignal(object)
    
    signalNewNavigationItem = pyqtSignal(dict) # id, name, type, (status), (refType), (refId), (schema)
    signalSelectNewNavigationItem = pyqtSignal(str) # deviceId
    signalSchemaAvailable = pyqtSignal(dict) # key, schema
    signalNavigationItemChanged = pyqtSignal(dict) # type, key
    signalNavigationItemSelectionChanged = pyqtSignal(str) # key
    signalDeviceInstanceChanged = pyqtSignal(dict, str)
    signalKillDevice = pyqtSignal(str) # deviceId
    signalKillServer = pyqtSignal(str) # serverId
    signalDeviceInstanceSchemaUpdated = pyqtSignal(str, str) # deviceId, schema
    
    signalRefreshInstance = pyqtSignal(str) # deviceId
    signalInitDevice = pyqtSignal(str, object) # deviceId, hash
    signalExecute = pyqtSignal(str, dict) # deviceId, slotName/arguments
    
    signalReconfigure = pyqtSignal(str, str, object) # deviceId, attributeId, attributeValue
    signalReconfigureAsHash = pyqtSignal(str, object) # deviceId, hash
    signalDeviceStateChanged = pyqtSignal(str, str) # fullDeviceKey, state
    signalConflictStateChanged = pyqtSignal(bool) # isBusy
    signalChangingState = pyqtSignal(bool) # isChanging
    
    signalInstanceGone = pyqtSignal(str, str) # path, parentPath
    
    signalNewVisibleDevice = pyqtSignal(str) # deviceId
    signalRemoveVisibleDevice = pyqtSignal(str) # deviceId

    signalLogDataAvailable = pyqtSignal(str) # logData
    signalErrorFound = pyqtSignal(str) # errorData
    signalAlarmFound = pyqtSignal(str) # alarmData
    signalWarningFound = pyqtSignal(str) # warningData
    
    signalCreateNewDeviceClassPlugin = pyqtSignal(str, str, str) # serverId, classId, newClassId
    
    signalGetClassSchema = pyqtSignal(str, str) # serverId, classId
    signalGetDeviceSchema = pyqtSignal(str) # deviceId


    def __init__(self):
        super(Notifier, self).__init__()
        pass
    

class Manager(Singleton):


    def init(self, *args, **kwargs):
        super(Manager, self).__init__()
        
        # Time between state updates
        self.__stateUpdateTime = QTime()
        self.__stateUpdateTime.start()
        
        # Tuple stores last state
        self.__lastState = None
        # Timer for state update, if last incoming state is a while ago
        self.__stateUpdateTimer = QTimer()
        self.__stateUpdateTimer.timeout.connect(self.onLastStateUpdateTimeOut)
        
        # Central hash
        self.__hash = Hash()
        
        # Map stores all keys and DataNofiers for editable widgets
        self.__keyNotifierMapEditableValue = dict()
        # Map stores all keys and DataNofiers for display widgets
        self.__keyNotifierMapDisplayValue = dict()
        self.__notifier = Notifier()
        
        # Dictionary to store instanceId of visible DEVICE_INSTANCEs with counter
        self.__visibleDevInsKeys = dict()
        
        # State, if initiate device is currently processed
        self.__isInitDeviceCurrentlyProcessed = False
        
        # Initiate database connection
        self.__sqlDatabase = SqlDatabase()
        self.__treemodel = NavigationHierarchyModel()


    def _hash(self):
        return self.__hash
    hash = property(fget=_hash)


    def _notifier(self):
        return self.__notifier
    notifier = property(fget=_notifier)


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
        if isinstance(value, QString):
            value = str(value)
        # Merge with central hash
        self.__hash.set(key, value)
        
        #print ""
        #print self.__hash
        #print ""


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
        

    def _getDeviceIdFromPath(self, path):
        splittedPath = str(path).split('device.')
        if len(splittedPath) < 2:
            # no device selected
            return None
        
        # Get deviceId
        deviceParameter = splittedPath[1].split('.configuration.')
        if len(deviceParameter) < 1:
            return None
        
        return deviceParameter[0]


    def newVisibleDevice(self, path):
        deviceId = self._getDeviceIdFromPath(path)
        if not deviceId:
            return
        
        deviceIdCount = self.__visibleDevInsKeys.get(deviceId)
        if deviceIdCount:
            self.__visibleDevInsKeys[deviceId] += 1
        else:
            self.__visibleDevInsKeys[deviceId] = 1
        if self.__visibleDevInsKeys[deviceId] == 1:
            self.__notifier.signalNewVisibleDevice.emit(deviceId)


    def removeVisibleDevice(self, path):
        deviceId = self._getDeviceIdFromPath(path)
        if not deviceId:
            return
        
        deviceIdCount = self.__visibleDevInsKeys.get(deviceId)
        if deviceIdCount:
            self.__visibleDevInsKeys[deviceId] -= 1
            if self.__visibleDevInsKeys[deviceId] == 0:
                self.__notifier.signalRemoveVisibleDevice.emit(deviceId)


    def _changeHash(self, devicePath, config, configChangeType=ConfigChangeTypes.DEVICE_INSTANCE_CURRENT_VALUES_CHANGED):
        address = devicePath
        # Go recursively through Hash
        self._r_checkHash(address, devicePath, config, configChangeType)


    def _r_checkHash(self, address, devicePath, config, configChangeType):
        topLevelKeys = config.keys()
        for key in topLevelKeys:
            value = config.get(key)

            if len(address) < 1:
                internalKey = key
            else:
                internalKey = address + "." + key
            
            #self._setFromPath(internalKey, value)
            
            # Check if DataNotifier for key available
            if configChangeType is ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED:
                dataNotifier = self._getDataNotifierEditableValue(internalKey)
                if dataNotifier is not None:
                    dataNotifier.signalUpdateComponent.emit(internalKey, value)
            elif configChangeType is ConfigChangeTypes.DEVICE_INSTANCE_CONFIG_CHANGED:
                dataNotifier = self._getDataNotifierEditableValue(internalKey)
                if dataNotifier is not None:
                    dataNotifier.signalUpdateComponent.emit(internalKey, value)
            elif configChangeType is ConfigChangeTypes.DEVICE_INSTANCE_CURRENT_VALUES_CHANGED:
                dataNotifier = self._getDataNotifierDisplayValue(internalKey)
                if dataNotifier is not None:
                    dataNotifier.signalUpdateComponent.emit(internalKey, value)
                
                # Notify editable widget of display value change
                dataNotifier = self._getDataNotifierEditableValue(internalKey)
                if dataNotifier is not None:
                    # Broadcast new displayValue to all editable widgets
                    dataNotifier.updateDisplayValue(internalKey, value)
                
                # Check state
                if key == "state":
                    # Store latest state as tuple
                    self.__lastState = (address, devicePath, value)
                    if self.__stateUpdateTime.elapsed() > 250:
                        # Update state when last state change happened before 250ms
                        self._triggerStateChange(address, devicePath, value)
                    else:
                        # Start timer for possible state update
                        self.__stateUpdateTimer.start(300)
                    # Start update state time again
                    self.__stateUpdateTime.start()
                        
            # More recursion in case of Hash type
            if isinstance(value, Hash):
                self._r_checkHash(internalKey, devicePath, value, configChangeType)
            elif isinstance(value, list):
                # TODO: needs to be implemented
                for i in xrange(len(value)):
                    internalKey = internalKey + "[" + str(i) + "]"
                    hashValue = value[i]


    def _triggerStateChange(self, address, devicePath, value):
        devicePath = devicePath.split('.configuration',1)[0]
        # Update GUI due to state changes
        if value == "Changing...":
            self.__notifier.signalChangingState.emit(True)
        else:
            self.__notifier.signalChangingState.emit(False)
            self.__notifier.signalDeviceStateChanged.emit(devicePath, value)


### Slots ###
    def onLastStateUpdateTimeOut(self):
        # Gets called when last state is not yet reflected in GUI
        if self.__stateUpdateTime.elapsed() > 250:
            if self.__lastState:
                address = self.__lastState[0]
                instanceId = self.__lastState[1]
                value = self.__lastState[2]
                self._triggerStateChange(address, instanceId, value)
        self.__stateUpdateTimer.stop()
        

    def onDeviceClassValueChanged(self, key, value):
        #print "onDeviceClassValueChanged", key, value
        self._setFromPath(key, value)
        
        dataNotifier = self._getDataNotifierEditableValue(key)
        if dataNotifier is not None:
            dataNotifier.signalUpdateComponent.emit(key, value)


    def onDeviceInstanceValueChanged(self, key, value):
        #print "onDeviceInstanceValueChanged", key, value
        # Safety conversion before hashing
        if isinstance(value, QString):
            value = str(value)
        
        self._setFromPath(key, value)

        dataNotifier = self._getDataNotifierEditableValue(key)
        if dataNotifier is not None:
            dataNotifier.signalUpdateComponent.emit(key, value)
        
        keys = str(key).split('.')
        deviceId = keys[1]
        parameterKey = keys[3]

        # Informs network
        self.__notifier.signalReconfigure.emit(deviceId, parameterKey, value)


    def onDeviceChangedAsHash(self, instanceKey, config):
        paths = config.paths()
        for path in paths:
            dataNotifier = self._getDataNotifierEditableValue(path)
            if dataNotifier is not None:
                dataNotifier.signalUpdateComponent.emit(path, config.get(path))
        
        self._mergeIntoHash(config)

        keys = str(instanceKey).split('.')
        instanceId = keys[1]
        self.__notifier.signalReconfigureAsHash.emit(instanceId, config.get(instanceKey + ".configuration"))


    def onConflictStateChanged(self, hasConflict):
        self.__notifier.signalConflictStateChanged.emit(hasConflict)


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
        
        self.__notifier.signalInitDevice.emit(serverId, config)
        self.__isInitDeviceCurrentlyProcessed = True


    def killDevice(self, deviceId):
        reply = QMessageBox.question(None, 'Message',
            "Do you really want to kill this instance?", QMessageBox.Yes |
            QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return

        # Remove device instance data from internal hash
        self._setFromPath("device." + deviceId, Hash())
        self.__notifier.signalKillDevice.emit(deviceId)


    def killServer(self, serverId):
        reply = QMessageBox.question(None, 'Message',
            "Do you really want to kill this instance?", QMessageBox.Yes |
            QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return
        
        # Remove device instance data from internal hash
        self._setFromPath("server." + serverId, Hash())
        self.__notifier.signalKillServer.emit(serverId)


### TODO: Temporary functions for scientific computing START ###
    def createNewDeviceClassId(self, classId):
        return self.__sqlDatabase.createNewDeviceClassId(classId)


    def createNewDeviceClassPlugin(self, serverId, classId, newDevClaId):
        self.__notifier.signalCreateNewDeviceClassPlugin.emit(serverId, classId, newDevClaId)
    
    
    def getSchemaByInternalKey(self, internalKey):
        levelIdTuple = self.__sqlDatabase.getLevelAndIdByInternalKey(internalKey)
        level = levelIdTuple[0]
        id = levelIdTuple[1]
        
        index = self.__treemodel.findIndex(level, id, 0)
        rowId = self.__treemodel.mappedRow(index)
        
        return self.__treemodel.getSchema(level, rowId)
### TODO: Temporary functions for scientific computing END ###


    def selectNavigationItemByKey(self, path):
        self.__notifier.signalNavigationItemSelectionChanged.emit(path)


    def executeCommand(self, itemInfo):
        # instanceId, name, arguments
        path = itemInfo.get(QString('path'))
        if path is None:
            path = itemInfo.get('path')
        keys = str(path).split('.')
        deviceId = keys[1]
        
        command = itemInfo.get(QString('command'))
        if command is None:
            command = itemInfo.get('command')
        
        args = itemInfo.get(QString('args'))
        if args is None:
            args = itemInfo.get('args')
        
        self.__notifier.signalExecute.emit(deviceId, dict(command=str(command), args=args))


    def onLogDataAvailable(self, logData):
        # Send message to logging panel
        self.__notifier.signalLogDataAvailable.emit(logData)


    def onErrorDataAvailable(self, errorData):
        # Send message to notification panel
        self.__notifier.signalErrorFound.emit(errorData)


    def onWarningDataAvailable(self, warningData):
        # Send message to notification panel
        self.__notifier.signalWarningFound.emit(warningData)


    def onAlarmDataAvailable(self, alarmData):
        # Send message to notification panel
        self.__notifier.signalAlarmFound.emit(alarmData)


    def onRefreshInstance(self, path):
        deviceId = self._getDeviceIdFromPath(path)
        if not deviceId:
            return
        self.__notifier.signalRefreshInstance.emit(deviceId)

   
    def onNewNavigationItem(self, itemInfo):
        self.__notifier.signalNewNavigationItem.emit(itemInfo)


    def onNewNode(self, itemInfo):
        itemInfo['type'] = NavigationItemTypes.HOST
        self.__notifier.signalNewNavigationItem.emit(itemInfo)


    def onNewDeviceServerInstance(self, itemInfo):
        itemInfo['type'] = NavigationItemTypes.SERVER
        itemInfo['refType'] = NavigationItemTypes.HOST
        self.__notifier.signalNewNavigationItem.emit(itemInfo)


    def onNewDeviceClass(self, itemInfo):
        className = itemInfo.get(QString('name'))
        if className is None:
            className = itemInfo.get('name')
        
        devSerInsId = itemInfo.get(QString('refId'))
        if devSerInsId is None:
            devSerInsId = itemInfo.get('refId')
        devSerInsName = self.__sqlDatabase.getDeviceServerInstanceById(devSerInsId)
        
        # Remove device class data from internal hash
        self._setFromPath(devSerInsName + "+" + className, Hash())
        
        itemInfo['type'] = NavigationItemTypes.CLASS
        itemInfo['refType'] = NavigationItemTypes.SERVER
        self.__notifier.signalNewNavigationItem.emit(itemInfo)


    def onNewDeviceInstance(self, itemInfo):
        itemInfo['type'] = NavigationItemTypes.DEVICE
        itemInfo['refType'] = NavigationItemTypes.CLASS
        self.__notifier.signalNewNavigationItem.emit(itemInfo)


    def onSelectNewDevice(self, deviceId):
        if self.__isInitDeviceCurrentlyProcessed is True:
            self.__notifier.signalSelectNewNavigationItem.emit(deviceId)
            self.__isInitDeviceCurrentlyProcessed = False


    def onUpdateDeviceServerInstance(self, itemInfo):
        # Remove old configurations of hash...
        name = itemInfo.get(QString('name'))
        if name is None:
            name = itemInfo.get('name')
        
        # Remove device server data from internal hash
        self._setFromPath(name, Hash())
        self.__notifier.signalUpdateDeviceServerInstance.emit(itemInfo)
        self.__notifier.signalUpdateDeviceServerInstanceFinished.emit(itemInfo)


    def onUpdateDeviceInstance(self, itemInfo):
        self.__notifier.signalUpdateDeviceInstance.emit(itemInfo)
        self.__notifier.signalUpdateDeviceInstanceFinished.emit(itemInfo)


    def onSchemaUpdated(self, instanceId, schema):
        self.__notifier.signalDeviceInstanceSchemaUpdated.emit(instanceId, schema)
        # Refresh needed
        self.onRefreshInstance(instanceId)


    def onSchemaAvailable(self, itemInfo):
        # Notify ConfigurationPanel
        self.__notifier.signalSchemaAvailable.emit(itemInfo)


    def onDeviceInstanceChanged(self, itemInfo, xml):
        self.__notifier.signalDeviceInstanceChanged.emit(itemInfo, xml)


    def openAsXml(self, filename, internalKey, configChangeType, classId):
        file = QFile(filename)
        if file.open(QIODevice.ReadOnly | QIODevice.Text) == False:
            return
        
        xmlContent = str()
        while file.atEnd() == False:
            xmlContent += str(file.readLine())

        # TODO: serializer needed?
        serializer = TextSerializerHash.create("Xml")
        config = serializer.load(xmlContent).get(classId)

        # TODO: Reload XSD in configuration panel
        # ...
        
        # Remove old data from internal hash
        self._setFromPath(internalKey, Hash())
        
        # Update internal hash with new data for internalKey
        path = internalKey + ".configuration"
        self._changeHash(path, config, configChangeType)
        self._mergeIntoHash(Hash(path, config))


    def onFileOpen(self, configChangeType, internalKey, classId=str()):
        filename = QFileDialog.getOpenFileName(None, "Open saved configuration", QDir.tempPath(), "XML (*.xml)")
        if filename.isEmpty():
            return
        
        file = QFile(filename)
        if file.open(QIODevice.ReadOnly | QIODevice.Text) == False:
            return
        
        self.openAsXml(filename, internalKey, configChangeType, classId)


    def saveAsXml(self, filename, classId, internalKey):
        path = str(internalKey + ".configuration")
        if self.__hash.has(path):
            config = Hash(classId, self.__hash.get(path))
        else:
            config = Hash()
        saveToFile(config, filename)

    
    def onSaveAsXml(self, classId, internalKey):
        filename = QFileDialog.getSaveFileName(None, "Save file as", QDir.tempPath(), "XML (*.xml)")
        if filename.isEmpty():
            return
        
        fi = QFileInfo(filename)
        if fi.suffix().isEmpty():
            filename += ".xml"

        self.saveAsXml(str(filename), classId, internalKey)


    def onNavigationItemChanged(self, itemInfo):
        self.__notifier.signalNavigationItemChanged.emit(itemInfo)


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
        self.__notifier.signalSystemTopologyChanged.emit(self.__hash)


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
        self.__notifier.signalSystemTopologyChanged.emit(self.__hash)
        # Update navigation treeviews
        self.__notifier.signalInstanceGone.emit(path, parentPath)


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
        self.__notifier.signalGetClassSchema.emit(serverId, classId)
        return None
    
    
    def handleDeviceSchema(self, deviceId, config):
        # Merge new configuration data into central hash
        self._mergeIntoHash(config)
        
        path = "device." + deviceId
        descriptionPath = path + ".description"
        self.onSchemaAvailable(dict(key=path, type=NavigationItemTypes.DEVICE, schema=config.get(descriptionPath)))


    def getDeviceSchema(self, deviceId):
        path = str("device." + deviceId + ".description")
        if self.__hash.has(path):
            return self.__hash.get(path)

        # Send network request
        self.__notifier.signalGetDeviceSchema.emit(deviceId)
        return None
        

    # TODO: This function must be thread-safe!!
    def handleConfigurationChanged(self, deviceId, config):
        configurationPath = "device." + deviceId + ".configuration"
        self._changeHash(configurationPath, config.get(configurationPath))
        
        # Merge new configuration data into central hash
        self._mergeIntoHash(config)

