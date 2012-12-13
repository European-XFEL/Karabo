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
from libkarabo import *
from navigationhierarchymodel import NavigationHierarchyModel
from singleton import Singleton
from sqldatabase import SqlDatabase

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class Notifier(QObject):
    """Class for signals which can not be integrated in Manager class"""
    # signals
    signalNewNavigationItem = pyqtSignal(dict) # id, name, type, (status), (refType), (refId), (schema)
    signalSelectNewNavigationItem = pyqtSignal(dict) # id, name, devClaId, schema
    signalSchemaAvailable = pyqtSignal(dict) # key, schema
    signalNavigationItemChanged = pyqtSignal(dict) # type, key
    signalDeviceInstanceChanged = pyqtSignal(dict, str)
    signalKillDeviceInstance = pyqtSignal(str, str) # devSrvIns, devInsId
    signalKillDeviceServerInstance = pyqtSignal(str) # devSrvIns
    signalDeviceInstanceSchemaUpdated = pyqtSignal(str, str) # instanceId, schema
    
    signalRefreshInstance = pyqtSignal(str) # instanceId
    signalInitDevice = pyqtSignal(str, object) # instanceId, hash
    signalSlotCommand = pyqtSignal(str, dict) # instanceId, slotName/arguments
    
    signalReconfigure = pyqtSignal(str, str, object) # instanceId, attributeId, attributeValue
    signalReconfigureAsHash = pyqtSignal(str, object) # hash/dict
    signalDeviceInstanceStateChanged = pyqtSignal(str, str) # fullDeviceKey, state
    signalConflictStateChanged = pyqtSignal(bool) # isBusy
    signalChangingState = pyqtSignal(bool) # isChanging
    signalErrorState = pyqtSignal(str, bool) # instanceId, isError
    
    signalUpdateDeviceServerInstance = pyqtSignal(dict)
    signalUpdateDeviceServerInstanceFinished = pyqtSignal(dict)
    signalUpdateDeviceInstance = pyqtSignal(dict)
    signalUpdateDeviceInstanceFinished = pyqtSignal(dict)
    
    signalNewVisibleDeviceInstance = pyqtSignal(str) # deviceInstanceId
    signalRemoveVisibleDeviceInstance = pyqtSignal(str) # deviceInstanceId

    signalLogDataAvailable = pyqtSignal(str) # logData
    signalErrorFound = pyqtSignal(str) # errorData
    signalAlarmFound = pyqtSignal(str) # alarmData
    signalWarningFound = pyqtSignal(str) # warningData
    
    signalCreateNewDeviceClassPlugin = pyqtSignal(str, str, str) # devSrvInsId, devClaId, newDevClaId

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
        # Connect signals to treemodel for later updates which just need to be done once
        self.__notifier.signalUpdateDeviceServerInstanceFinished.connect(self.__treemodel.onUpdateDeviceServerInstance)
        self.__notifier.signalUpdateDeviceInstanceFinished.connect(self.__treemodel.onUpdateDeviceInstance)


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

    
    def getFromPathAsHash(self, key):
        return self.__hash.getFromPath(str(key))


    def _setFromPath(self, key, value):
        # pass key and value as list (immutable, important for ungoing procedures)
        #print "key:", key, "value:", value
        key = str(key)
        # Safety conversion before hashing
        if isinstance(value, QString):
            value = str(value)
        # Merge with central hash
        self.__hash.setFromPath(key, value)


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
        

    def newVisibleDeviceInstance(self, internalKey):
        keys = str(internalKey).split('.', 1)
        devInsId = keys[0]
        devInsCount = self.__visibleDevInsKeys.get(devInsId)
        if devInsCount:
            self.__visibleDevInsKeys[devInsId] += 1
        else:
            self.__visibleDevInsKeys[devInsId] = 1
        if self.__visibleDevInsKeys[devInsId] == 1:
            self.__notifier.signalNewVisibleDeviceInstance.emit(devInsId)


    def removeVisibleDeviceInstance(self, internalKey):
        keys = str(internalKey).split('.', 1)
        devInsId = keys[0]
        devInsCount = self.__visibleDevInsKeys.get(devInsId)
        if devInsCount:
            self.__visibleDevInsKeys[devInsId] -= 1
            if self.__visibleDevInsKeys[devInsId] == 0:
                self.__notifier.signalRemoveVisibleDeviceInstance.emit(devInsId)


    def _changeHash(self, instanceId, config, configChangeType=ConfigChangeTypes.DEVICE_INSTANCE_CURRENT_VALUES_CHANGED):
        address = instanceId
        # Go recursively through Hash
        self._r_checkHash(address, instanceId, config, configChangeType)


    def _r_checkHash(self, address, instanceId, config, configChangeType):
        topLevelKeys = config.keys()
        for key in topLevelKeys:
            value = config.get(key)

            if len(address) < 1:
                internalKey = key
            else:
                internalKey = address + "." + key
            
            self._setFromPath(internalKey, value)
            
            # Check if DataNotifier for key available
            if configChangeType is ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED:
                dataNotifier = self._getDataNotifierEditableValue(internalKey)
                if dataNotifier is not None:
                    dataNotifier.signalUpdateComponent.emit(internalKey, value)
            elif configChangeType is ConfigChangeTypes.DEVICE_INSTRANCE_CONFIG_CHANGED:
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
                    self.__lastState = (address, instanceId, value)
                    if self.__stateUpdateTime.elapsed() > 250:
                        # Update state when last state change happened before 250ms
                        self._triggerStateChange(address, instanceId, value)
                    else:
                        # Start timer for possible state update
                        self.__stateUpdateTimer.start(300)
                    # Start update state time again
                    self.__stateUpdateTime.start()
                        
            # More recursion in case of Hash type
            if isinstance(value, Hash):
                self._r_checkHash(internalKey, instanceId, value, configChangeType)
            elif isinstance(value, list):
                # TODO: needs to be implemented
                for i in xrange(len(value)):
                    internalKey = internalKey + "[" + str(i) + "]"
                    hashValue = value[i]


    def _triggerStateChange(self, address, instanceId, value):
        # Update GUI due to state changes
        if value == "Changing...":
            self.__notifier.signalChangingState.emit(True)
        else:
            if ("Error" in value) or ("error" in value):
                self.__notifier.signalErrorState.emit(instanceId, True)
            else:
                self.__notifier.signalErrorState.emit(instanceId, False)
            self.__notifier.signalChangingState.emit(False)
            self.__notifier.signalDeviceInstanceStateChanged.emit(address, value)


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
        

    # TODO: This function must be thread-safe!!    
    def onConfigChanged(self, instanceId, config):
        # This function is called from the network
        self._changeHash(instanceId, config)


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
        
        keys = str(key).split('.', 1)
        instanceId = keys[0]
        attributeKey = keys[1]

        # Informs network
        self.__notifier.signalReconfigure.emit(instanceId, attributeKey, value)


    def onDeviceInstanceChangedAsHash(self, instanceKey, config):
        leaves = config.getLeaves() # list with full key names
        for leaf in leaves :
            if len(leaf) < 1: continue
            
            key = instanceKey + "." + leaf
            value = config.getFromPath(leaf)
            self._setFromPath(key, value)

            dataNotifier = self._getDataNotifierEditableValue(key)
            if dataNotifier is not None:
                dataNotifier.signalUpdateComponent.emit(key, value)
        
        keys = str(instanceKey).split('.', 1)
        instanceId = keys[0]
        #classId = keys[1]
        self.__notifier.signalReconfigureAsHash.emit(instanceId, config)


    def onConflictStateChanged(self, hasConflict):
        self.__notifier.signalConflictStateChanged.emit(hasConflict)


    def initDevice(self, devSerInsId, devClaId, internalKey):
        #print "initDevice", internalKey
        #print self.__hash
        
        h = Hash()
        if self.__hash.hasFromPath(internalKey):
            h = self.__hash.getFromPath(internalKey)
        
        # TODO: Remove dirty hack for scientific computing again!!!
        croppedDevClaId = devClaId.split("-")
        devClaId = croppedDevClaId[0]
        
        config = Hash(devClaId, h)
        
        self.__notifier.signalInitDevice.emit(devSerInsId, config)
        self.__isInitDeviceCurrentlyProcessed = True


    def killDeviceInstance(self, devSrvInsId, devInsId):
        reply = QMessageBox.question(None, 'Message',
            "Do you really want to kill this instance?", QMessageBox.Yes |
            QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return

        # Remove device instance data from internal hash
        self._setFromPath(devInsId, Hash())
        self.__notifier.signalKillDeviceInstance.emit(devSrvInsId, devInsId)


    def killDeviceServerInstance(self, devSrvInsId):
        reply = QMessageBox.question(None, 'Message',
            "Do you really want to kill this instance?", QMessageBox.Yes |
            QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.No:
            return
        
        self.__notifier.signalKillDeviceServerInstance.emit(devSrvInsId)


### TODO: Temporary functions for scientific computing START ###
    def createNewDeviceClassId(self, devClaId):
        return self.__sqlDatabase.createNewDeviceClassId(devClaId)


    def createNewDeviceClassPlugin(self, devSrvInsId, devClaId, newDevClaId):
        self.__notifier.signalCreateNewDeviceClassPlugin.emit(devSrvInsId, devClaId, newDevClaId)
    
    
    def getSchemaByInternalKey(self, internalKey):
        levelIdTuple = self.__sqlDatabase.getLevelAndIdByInternalKey(internalKey)
        level = levelIdTuple[0]
        id = levelIdTuple[1]
        
        index = self.__treemodel.findIndex(level, id, 0)
        rowId = self.__treemodel.mappedRow(index)
        
        return self.__treemodel.getSchema(level, rowId)


    def selectNavigationItemByInternalKey(self, internalKey):
        levelIdTuple = self.__sqlDatabase.getLevelAndIdByInternalKey(internalKey)
        level = levelIdTuple[0]
        id = levelIdTuple[1]
        
        name = None
        keys = internalKey.split('+', 1)
        if len(keys) == 2:
            # Internal key for device class
            name = keys[1]
        else:
            # Internal key for device instance
            keys = internalKey.split('.', 1)
            internalKey = keys[0]
            name = internalKey

        self.onNavigationItemChanged(dict(key=internalKey, name=name, level=level, rowId=id, column=1))
### TODO: Temporary functions for scientific computing END ###


    def slotCommand(self, itemInfo):
        # instanceId, name, arguments
        internalKey = itemInfo.get(QString('internalKey'))
        if internalKey is None:
            internalKey = itemInfo.get('internalKey')
        keys = str(internalKey).rsplit('.', 3)
        instanceId = keys[0]
        
        name = itemInfo.get(QString('name'))
        if name is None:
            name = itemInfo.get('name')
        
        args = itemInfo.get(QString('args'))
        if args is None:
            args = itemInfo.get('args')
        
        self.__notifier.signalSlotCommand.emit(instanceId, dict(name=str(name), args=args))


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


    def onRefreshInstance(self, internalKey):
        instanceId = str(internalKey).rsplit('.', 1)[0]
        self.__notifier.signalRefreshInstance.emit(instanceId)

   
    def onNewNavigationItem(self, itemInfo):
        self.__notifier.signalNewNavigationItem.emit(itemInfo)


    def onNewNode(self, itemInfo):
        itemInfo['type'] = NavigationItemTypes.NODE
        self.__notifier.signalNewNavigationItem.emit(itemInfo)


    def onNewDeviceServerInstance(self, itemInfo):
        itemInfo['type'] = NavigationItemTypes.DEVICE_SERVER_INSTANCE
        itemInfo['refType'] = NavigationItemTypes.NODE
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
        
        itemInfo['type'] = NavigationItemTypes.DEVICE_CLASS
        itemInfo['refType'] = NavigationItemTypes.DEVICE_SERVER_INSTANCE
        self.__notifier.signalNewNavigationItem.emit(itemInfo)


    def onNewDeviceInstance(self, itemInfo):
        itemInfo['type'] = NavigationItemTypes.DEVICE_INSTANCE
        itemInfo['refType'] = NavigationItemTypes.DEVICE_CLASS
        self.__notifier.signalNewNavigationItem.emit(itemInfo)


    def onSelectNewDeviceInstance(self, itemInfo):
        if self.__isInitDeviceCurrentlyProcessed is True:
            self.__notifier.signalSelectNewNavigationItem.emit(itemInfo)
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
        key = itemInfo.get(QString('key'))
        if key is None:
            key = itemInfo.get('key')
        
        # Notify ConfigurationPanel
        self.__notifier.signalSchemaAvailable.emit(itemInfo)


    def onDeviceInstanceChanged(self, itemInfo, xml):
        self.__notifier.signalDeviceInstanceChanged.emit(itemInfo, xml)


    def openAsXml(self, filename, internalKey, configChangeType, devClaId):
        file = QFile(filename)
        if file.open(QIODevice.ReadOnly | QIODevice.Text) == False:
            return
        
        xmlContent = str()
        while file.atEnd() == False:
            xmlContent += str(file.readLine())

        # TODO: serializer needed?
        serializer = FormatHash.create("Xml", Hash())
        config = serializer.unserialize(xmlContent).getFromPath(devClaId)

        # TODO: Reload XSD in configuration panel
        # ...
        
        # Remove old data from internal hash
        self._setFromPath(internalKey, Hash())
        # Update internal hash with new data for internalKey
        self._changeHash(internalKey, config, configChangeType)


    def onFileOpen(self, configChangeType, internalKey, devClaId=str()):
        filename = QFileDialog.getOpenFileName(None, "Open saved configuration", QDir.tempPath(), "XML (*.xml)")
        if filename.isEmpty():
            return
        
        file = QFile(filename)
        if file.open(QIODevice.ReadOnly | QIODevice.Text) == False:
            return
        
        self.openAsXml(filename, internalKey, configChangeType, devClaId)


    def saveAsXml(self, filename, devClaId, internalKey):
        writeHash = Hash("TextFile.filename", str(filename))
        writeHash.setFromPath("TextFile.format.Xml.indentation", 1)
        writeHash.setFromPath("TextFile.format.Xml.printDataType", True)
        xmlWriter = WriterHash.create(writeHash)
        xmlWriter.write(Hash(devClaId, self.getFromPathAsHash(internalKey)))

    
    def onSaveAsXml(self, devClaId, internalKey):
        filename = QFileDialog.getSaveFileName(None, "Save file as", QDir.tempPath(), "XML (*.xml)")
        if filename.isEmpty() :
            return
        
        fi = QFileInfo(filename)
        if fi.suffix().isEmpty():
            filename += ".xml"

        self.saveAsXml(filename, devClaId, internalKey)


    def onNavigationItemChanged(self, itemInfo):
        self.__notifier.signalNavigationItemChanged.emit(itemInfo)


    # TODO: needs to be implemented
    def onReloadXsd(self, deviceServer, deviceId):
        print "Manager, onReloadXsd", deviceServer, deviceId
        #mainWindow signalReloadXsd
        # onXsdAvailable.emit()
        pass

