#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on April 7, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the project related datastructure.
"""

__all__ = ["Device", "DeviceGroup", "GuiProject", "Macro", "Category"]

from karabo.middlelayer_api.project import (
    BaseDevice, BaseDeviceGroup, BaseMacro, Monitor, Project,
    ProjectConfiguration)
from karabo.middlelayer import AccessMode, Hash, XMLParser, XMLWriter
from karabo_gui.configuration import Configuration
from karabo_gui.messagebox import MessageBox
from karabo_gui.network import network
from karabo_gui.scene import Scene
from karabo_gui.topology import getClass, getDevice, Manager

from PyQt4.QtCore import pyqtSignal, QObject

import csv
from datetime import datetime
from zipfile import ZipFile


class BaseConfiguration(Configuration):
    signalConfigurationModified = pyqtSignal(bool)

    def __init__(self, deviceId, type, descriptor=None):
        Configuration.__init__(self, deviceId, type, descriptor)

        # This flag states whether the descriptor was checked already
        # This should only happen once when the device was selected the first
        # time in the project view
        self.descriptorRequested = False
        self._initConfig = None

    @property
    def initConfig(self):
        return self._initConfig

    @initConfig.setter
    def initConfig(self, config):
        self._initConfig = config
        # Merge initConfig, if descriptor is not None
        self.mergeInitConfig()

    def mergeInitConfig(self):
        """
        This function merges the \self.initConfig into the Configuration.
        This is only possible, if the descriptor has been set before.
        """
        if self.descriptor is None:
            return

        # Set default values for configuration
        self.setDefault()
        if self._initConfig is not None:
            self.fromHash(self._initConfig)

    def fromXml(self, xmlString):
        """
        This function loads the corresponding XML file of this configuration.
        """
        self.fromHash(XMLParser().read(xmlString))

    def toXml(self):
        """
        This function returns the configurations' XML file as a string.
        """
        if self.descriptor is not None:
            config, _ = self.toHash()  # Ignore returned attributes
        else:
            config = self._initConfig

        return XMLWriter().write(Hash(self.classId, config))

    def checkClassSchema(self):
        if self.descriptorRequested is True:
            return
        
        # Get class configuration
        conf = getClass(self.serverId, self.classId)

        conf.signalNewDescriptor.connect(self.onNewDescriptor)
        if conf.descriptor is not None:
            self.onNewDescriptor(conf)

        self.descriptorRequested = True

    def onNewDescriptor(self, conf):
        if self.descriptor is not None:
            self.redummy()
        self.descriptor = conf.descriptor
        self.mergeInitConfig()
        Manager().onShowConfiguration(self)


class Device(BaseDevice, BaseConfiguration):
    signalDeviceNeedsUpdate = pyqtSignal(object)

    def __init__(self, serverId, classId, deviceId, ifexists, descriptor=None):
        BaseConfiguration.__init__(self, deviceId, "projectClass", descriptor)
        BaseDevice.__init__(self, serverId, classId, deviceId, ifexists)

        actual = getDevice(deviceId)
        actual.signalStatusChanged.connect(self.onStatusChanged)
        self.onStatusChanged(actual, actual.status, actual.error)

    def onStatusChanged(self, conf, status, error):
        """ this method gets the status of the corresponding real device,
        and finds out the gory details for this project device """
        self.error = error

        manager = Manager()
        if manager.systemHash is None:
            self.status = "offline"
            return

        if status == "offline":
            try:
                attrs = manager.systemHash[
                    "server.{}".format(self.serverId), ...]
            except KeyError:
                self.status = "noserver"
            else:
                if self.classId not in attrs.get("deviceClasses", []):
                    self.status = "noplugin"
                else:
                    self.status = "offline"
            if conf.classId is None and conf.serverId is None:
                conf.classId = self.classId
                conf.serverId = self.serverId
                if conf.descriptor is None:
                    cls = getClass(self.serverId, self.classId)
                    if cls.descriptor is None:
                        cls.signalNewDescriptor.connect(conf.onClassDescriptor)
                    else:
                        conf.descriptor = cls.descriptor
        else:
            if (conf.classId == self.classId and
                    conf.serverId == self.serverId):
                self.status = status
            else:
                self.status = "incompatible"

    def onNewDescriptor(self, conf):
        BaseConfiguration.onNewDescriptor(self, conf)
        self.signalDeviceNeedsUpdate.emit(self)

    def addVisible(self):
        realDevice = getDevice(self.id)
        realDevice.addVisible()

    def removeVisible(self):
        realDevice = getDevice(self.id)
        realDevice.removeVisible()

    def shutdown(self):
        self.project.shutdown(self)


class DeviceGroup(BaseDeviceGroup, BaseConfiguration):
    signalDeviceGroupStatusChanged = pyqtSignal()


    def __init__(self, id, serverId, classId, ifexists, type="deviceGroupClass"):
        BaseConfiguration.__init__(self, id, type)
        BaseDeviceGroup.__init__(self, serverId, classId, id, ifexists)

        # If all device are online there is another deviceGroup to represent this
        self.instance = None

    def createInstance(self):
        """
        The online instance of the deviceGroup is created once and returned.
        """
        if self.instance is not None:
            return self.instance

        self.instance = DeviceGroup(self.id, self.serverId, self.classId, self.ifexists, "deviceGroup")
        self.instance.devices = self.devices
        self.instance.project = self.project

        return self.instance

    def onUpdateDevice(self, device):
        """
        This slot is setting the configuration of this device group to the given
        \device.
        """
        if device not in self.devices:
            return
        
        if self.descriptor is None:
            device.initConfig = self.initConfig
        else:
            hsh, _ = self.toHash()  # Ignore returned attributes
            device.initConfig = hsh

    def checkDeviceSchema(self):
        """
        The device schema of an online device group is checked.
        """
        if self.descriptorRequested is True:
            return
        
        # Get device configurations
        for device in self.devices:
            conf = getDevice(device.id)
            
            conf.signalNewDescriptor.connect(self.onNewDeviceDescriptor)
            if conf.descriptor is not None:
                self.onNewDeviceDescriptor(conf)
            # Do this only for first device - it is expected that the deviceSchema
            # is equal for all group devices
            break
        
        self.descriptorRequested = True

    def onNewDescriptor(self, conf):
        """
        This slot is called whenever a new descriptor for a requested class schema
        is available which belongs to this device group.
        """
        BaseConfiguration.onNewDescriptor(self, conf)
        # Recursively connect deviceGroupClass boxes to boxes of devices
        for d in self.devices:
            d.onNewDescriptor(conf)
            self.connectOtherBox(d)

    def onNewDeviceDescriptor(self, conf):
        """
        This slot is called whenever a new descriptor for a requested device schema
        is available which belongs to the instance of this device group.
        """
        BaseConfiguration.onNewDescriptor(self, conf)
        # Recursively copy device boxes to device group
        for d in self.devices:
            actual = getDevice(d.id)
            # Copy from online device
            self.copyFrom(actual, lambda descr: descr.accessMode == AccessMode.RECONFIGURABLE)

    def onStatusChanged(self, conf, status, error):
        change = False
        if self.status != status:
            self.status = status
            change = True
        if self.error != error:
            self.error = error
            change = True
        
        if change:
            self.signalDeviceGroupStatusChanged.emit()

    def isOnline(self):
        return all(d.isOnline() for d in self.devices)

    def addVisible(self):
        for device in self.devices:
            realDevice = getDevice(device.id)
            realDevice.addVisible()

    def removeVisible(self):
        for device in self.devices:
            device.removeVisible()


class GuiProject(Project, QObject):
    Device = Device
    DeviceGroup = DeviceGroup

    signalProjectModified = pyqtSignal(object)
    signalSelectObject = pyqtSignal(object)
    signalDeviceSelected = pyqtSignal(str, object)
    
    signalDeviceAdded = pyqtSignal(object)
    signalDeviceInserted = pyqtSignal(int, object)
    signalDeviceGroupAdded = pyqtSignal(object)
    signalDeviceGroupInserted = pyqtSignal(int, object)
    signalSceneAdded = pyqtSignal(object)
    signalSceneInserted = pyqtSignal(int, object)
    signalConfigurationAdded = pyqtSignal(str, object)
    signalConfigurationInserted = pyqtSignal(int, str, object)
    #signalResourceAdded = pytqtSignal()
    signalMacroAdded = pyqtSignal(object)
    signalMacroChanged = pyqtSignal(object)
    signalMonitorAdded = pyqtSignal(object)
    signalMonitorInserted = pyqtSignal(int, object)
    
    signalRemoveObject = pyqtSignal(object)

    def __init__(self, filename):
        super(GuiProject, self).__init__(filename)

        self.modules = {} # the modules in the macro context
        
        # States whether the project was changed or not
        self.isModified = False

    def setModified(self, isModified):
        """
        The project was modified and the associated modelview needs to be
        updated.
        
        isModified - states, whether modification was done
        """
        if self.isModified == isModified:
            return
        
        self.isModified = isModified
        self.signalProjectModified.emit(self)

    def setupDeviceToProject(self, device):
        self.setModified(True)
        # Connect device to project to get configuration changes
        device.signalConfigurationModified.connect(self.setModified)

    def addDevice(self, device):
        super(GuiProject, self).addDevice(device)
        self.signalDeviceAdded.emit(device)
        self.setupDeviceToProject(device)

    def insertDevice(self, index, device):
        super(GuiProject, self).insertDevice(index, device)
        self.signalDeviceInserted.emit(index, device)
        self.setupDeviceToProject(device)

    def newDevice(self, serverId, classId, deviceId, ifexists):
        """
        A new device with the given parameters is created, added to the
        project and returned.
        """
        device = Device(serverId, classId, deviceId, ifexists)
        self.addDevice(device)
        
        return device

    def setupDeviceGroupToProject(self, deviceGroup):
        self.setModified(True)
        # Connect device to project to get configuration changes
        deviceGroup.signalConfigurationModified.connect(self.setModified)
        for device in deviceGroup.devices:
            device.signalDeviceNeedsUpdate.connect(deviceGroup.onUpdateDevice)
            # Get correct status from devices
            actual = getDevice(device.id)
            actual.signalStatusChanged.connect(deviceGroup.onStatusChanged)
            deviceGroup.onStatusChanged(actual, actual.status, actual.error)

    def addDeviceGroup(self, deviceGroup):
        super(GuiProject, self).addDeviceGroup(deviceGroup)
        self.signalDeviceGroupAdded.emit(deviceGroup)
        self.setupDeviceGroupToProject(deviceGroup)

    def insertDeviceGroup(self, index, deviceGroup):
        super(GuiProject, self).insertDeviceGroup(index, deviceGroup)
        self.signalDeviceGroupInserted.emit(index, deviceGroup)
        self.setupDeviceGroupToProject(deviceGroup)

    def newDeviceGroup(self, groupId, serverId, classId, ifexists, prefix,
                       start, end):
        """
        This function creates a new device group with the given parameters,
        adds it to this project and returns it.
        """
        deviceGroup = self.createDeviceGroup(groupId, serverId, classId,
                                             ifexists, prefix, start, end)
        self.addDeviceGroup(deviceGroup)
        
        return deviceGroup

    def createDeviceGroup(self, groupId, serverId, classId, ifexists,
                          prefix, start, end):
        """
        This function only creates a device group with the given parameter and
        returns it.
        
        It does not yet append it to this project.
        """
        deviceGroup = DeviceGroup(groupId, serverId, classId, ifexists)
        
        for index in range(start, end+1):
            id = "{}{}".format(prefix, index)
            device = Device(serverId, classId, id, ifexists)
            device.signalDeviceNeedsUpdate.connect(deviceGroup.onUpdateDevice)
            # Get correct status from devices
            actual = getDevice(device.id)
            actual.signalStatusChanged.connect(deviceGroup.onStatusChanged)
            deviceGroup.onStatusChanged(actual, actual.status, actual.error)
            deviceGroup.addDevice(device)
        return deviceGroup

    def addScene(self, scene):
        self.scenes.append(scene)
        self.signalSceneAdded.emit(scene)
        self.setModified(True)

    def insertScene(self, index, scene):
        """
        Insert \scene at given \index and update project model.
        """
        super(GuiProject, self).insertScene(index, scene)
        self.signalSceneInserted.emit(index, scene)

    def replaceScene(self, scene):
        """
        The \scene is copied into a \newScene. This newScene object replaces the
        old one.
        """
        currentlyModified = self.isModified
        # Create a new Scene instance from the data in the project.
        newScene = self.rereadScene(scene.filename)

        # Remove old scene from project
        index = self.remove(scene)
        # Insert \newScene
        self.insertScene(index, newScene)
        self.setModified(currentlyModified)

    def rereadScene(self, filename):
        """ Create a new Scene instance by reading the scene XML/SVG data from
        the project file and constructing a new object.
        """
        url = self.getSceneURL(filename)
        try:
            sceneXmlData = self.getURL(url)
        except KeyError:
            sceneXmlData = b''

        scene = Scene(self, filename)
        if sceneXmlData:
            scene.fromXml(sceneXmlData)

        return scene

    def addConfiguration(self, deviceId, configuration):
        super(GuiProject, self).addConfiguration(deviceId, configuration)
        self.signalConfigurationAdded.emit(deviceId, configuration)
        self.setModified(True)

    def insertConfiguration(self, index, deviceId, configuration):
        super(GuiProject, self).addConfiguration(deviceId, configuration, index)
        self.signalConfigurationInserted.emit(index, deviceId, configuration)
        self.setModified(True)

    def removeConfiguration(self, deviceId, configuration):
        """
        The \configuration of the given \deviceId should be removed from this
        project.
        
        Returns \index of the configuration list.
        """
        index = super(GuiProject, self).removeConfiguration(deviceId, configuration)
        self.signalRemoveObject.emit(configuration)
        self.setModified(True)
        return index

    def addMacro(self, macro):
        self.macros[macro.name] = macro
        self.signalMacroAdded.emit(macro)
        self.setModified(True)

    def addMonitor(self, monitor):
        super(GuiProject, self).addMonitor(monitor)
        self.signalMonitorAdded.emit(monitor)
        self.setModified(True)

    def insertMonitor(self, index, monitor):
        """
        Insert ``monitor`` at given ``index`` and update project.
        """
        super(GuiProject, self).insertMonitor(self, index, monitor)
        self.signalMonitorInserted.emit(index, monitor)
        self.setModified(True)

    def addResource(self, category, data):
        #self.signalResourceAdded.emit(category, data)
        self.setModified(True)
        return super(GuiProject, self).addResource(category, data)

    def remove(self, object):
        """
        The \object should be removed from this project.
        
        Returns \index of the object in the list.
        """
        self.signalRemoveObject.emit(object)
        self.setModified(True)
        if isinstance(object, Configuration):
            index = self.devices.index(object)
            self.devices.pop(index)
            return index
        elif isinstance(object, Scene):
            index = self.scenes.index(object)
            self.scenes.pop(index)
            return index
        elif isinstance(object, Macro):
            del self.macros[object.name]
            return -1
        elif isinstance(object, Monitor):
            index = self.monitors.index(object)
            self.monitors.pop(index)
            return index

    def saveProject(self, filename=None):
        """ Wrapped version of zip() which catches all file saving exceptions
        """
        try:
             self.zip(filename=filename)
        except Exception as e:
            msg = "Saving project <b>{}</b> failed!<br>Reason: '{}'".format(
                self.name, e)
            MessageBox().showError(msg, "Saving failed")

    def unzip(self, factories=None):
        objFactories = {
            'Device': Device,
            'DeviceGroup': DeviceGroup,
            'Macro': Macro,
            'Monitor': Monitor,
            'ProjectConfiguration': ProjectConfiguration,
            'Scene': Scene,
        }
        if factories is not None:
            objFactories.update(factories)

        super(GuiProject, self).unzip(factories=objFactories)
        self.setModified(False)

    def zip(self, filename=None):
        """ This method saves this project as a zip file.
        """
        super(GuiProject, self).zip(filename=filename)
        self.setModified(False)

    def _instantiateDevice(self, device):
        manager = Manager()
        if device.isOnline():
            if device.ifexists == "ignore":
                return
            elif device.ifexists == "restart":
                # This might take a while...
                manager.shutdownDevice(device.id, False)

        if device.descriptor is None:
            config = device.initConfig
        else:
            hsh, _ = device.toHash()  # Ignore returned attributes
            config = hsh

        manager.initDevice(device.serverId, device.classId, device.id, config)

    def instantiate(self, device):
        """
        This function instantiates the \device.
        """
        if isinstance(device, Device):
            self._instantiateDevice(device)
        elif isinstance(device, DeviceGroup):
            for gd in device.devices:
                # In case this device of the group was never clicked, there might
                # be no configuration set... To prevent this, use configuration
                # of device group
                if device.descriptor is None:
                    gd.initConfig = device.initConfig
                else:
                    hsh, _ = device.toHash()  # Ignore returned attributes
                    gd.initConfig = hsh
                self._instantiateDevice(gd)

    def instantiateAll(self):
        """
        This function instantiates all project devices.
        """
        for d in self.devices:
            self.instantiate(d)

    def _shutdownDevice(self, device, showConfirm):
        Manager().shutdownDevice(device.id, showConfirm)

    def shutdown(self, device, showConfirm=True):
        """
        This function shuts down \device.
        """
        if isinstance(device, Device):
            self._shutdownDevice(device, showConfirm)
        elif isinstance(device, DeviceGroup):
            for gd in device.devices:
                self._shutdownDevice(gd, False)

    def shutdownAll(self):
        """
        This function shuts down all project devices.
        """
        for d in self.devices:
            self.shutdown(d, False)

    def startMonitoring(self):
        """
        Monitoring is started and all necessary variables are initiated.
        """
        if self.isMonitoring:
            print("Monitoring is already started.")
            return
        
        self.isMonitoring = True
        
        self.monitorFile = open(self.monitorFilename, "a")
        self.monitorWriter = csv.writer(self.monitorFile)
        
        self.monitorBoxes = []
        monitorString = []
        for m in self.monitors:
            # Connect box changes of properties to be monitored
            deviceId = m.config.get("deviceId")
            deviceProperty = m.config.get("deviceProperty")
            
            conf = getDevice(deviceId)
            box = conf.getBox(deviceProperty.split("."))
            self.monitorBoxes.append(box)
            box.addVisible()
            box.signalUpdateComponent.connect(self.onMonitorValueChanged)
            
            monitorString.append("{}[{}{}]".format(m.config.get("name"),
                                             m.config.get("metricPrefixSymbol"),
                                             m.config.get("unitSymbol")))
        
        self.monitorWriter.writerow(["#timestamp"] + monitorString)
        if self.monitorInterval > 0:
            self.monitorTimer = self.startTimer(self.monitorInterval * 1000)
        else:
            self.monitorTimer = None

    def stopMonitoring(self):
        """
        Monitoring is stopped and all variables are reset.
        """
        if not self.isMonitoring:
            print("Monitoring is already stopped.")
            return
        
        self.isMonitoring = False
        
        for b in self.monitorBoxes:
            b.removeVisible()
            b.signalUpdateComponent.disconnect(self.onMonitorValueChanged)
        self.monitorBoxes = []

        if self.monitorTimer is not None:
            self.killTimer(self.monitorTimer)
        self.monitorFile.close()

    def timerEvent(self, event=None, timestamp=None):
        """
        Write to monitor file.
        """
        if self.monitorWriter is None:
            return
        
        if timestamp is None:
            timestamp = datetime.now().isoformat()
        else:
            timestamp = timestamp.toLocal()
        
        self.monitorWriter.writerow([timestamp] + [b.value for b in self.monitorBoxes])

    def onMonitorValueChanged(self, box, value, timestamp):
        """
        Whenever the value of the given box is changed, write to monitoring file,
        under certain conditions.
        """
        if self.monitorInterval == 0 and self.isMonitoring and box in self.monitorBoxes:
            self.timerEvent(None, timestamp)


class Macro(BaseMacro):
    def __init__(self, project, name):
        super(Macro, self).__init__(project, name)
        self.macros = {}
        self.instances = []

    def run(self):
        if self.editor is None:
            code = self.load()
        else:
            code = self.editor.edit.toPlainText()
        h = Hash("code", code,
                 "project", self.project.name,
                 "module", self.name)
        network.onInitDevice("Karabo_MacroServer", "MetaMacro", self.instanceId, h)

    def load(self):
        with ZipFile(self.project.filename, "r") as zf:
            return zf.read("macros/{}.py".format(self.name)).decode("utf8")


class Category(object):
    """
    This class represents a project category and is only used to have an object
    for the view items.
    """
    def __init__(self, displayName):
        super(Category, self).__init__()
        
        self.displayName = displayName
