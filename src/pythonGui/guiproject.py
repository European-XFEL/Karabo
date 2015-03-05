
#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on April 7, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the project related datastructure.
"""

__all__ = ["Device", "DeviceGroup", "GuiProject", "Macro", "Category"]


from configuration import Configuration
from scene import Scene
from schema import Schema
from karabo.enums import AccessMode
from karabo.hash import Hash, XMLParser, XMLWriter
from karabo.hashtypes import StringList
from karabo.project import Project, BaseDevice, BaseDeviceGroup
import karabo
import manager
from network import network

from PyQt4.QtCore import pyqtSignal, QObject
from PyQt4.QtGui import QMessageBox

from importlib import import_module, reload
from importlib.util import cache_from_source
import marshal
import os.path
import sys
from tempfile import NamedTemporaryFile
import types
from zipfile import ZipFile, ZIP_DEFLATED


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
        if self.descriptor is None: return

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
            config = self.toHash()
        else:
            config = self._initConfig

        return XMLWriter().write(Hash(self.classId, config))


    def checkClassSchema(self):
        if self.descriptorRequested is True:
            return
        
        # Get class configuration
        conf = manager.getClass(self.serverId, self.classId)

        conf.signalNewDescriptor.connect(self.onNewDescriptor)
        if conf.descriptor is not None:
            self.onNewDescriptor(conf)

        self.descriptorRequested = True


    def onNewDescriptor(self, conf):
        if self.descriptor is not None:
            self.redummy()
        self.descriptor = conf.descriptor
        self.mergeInitConfig()
        manager.Manager().onShowConfiguration(self)


class Device(BaseDevice, BaseConfiguration):
    signalDeviceNeedsUpdate = pyqtSignal(object)

    def __init__(self, serverId, classId, deviceId, ifexists, descriptor=None):
        BaseConfiguration.__init__(self, deviceId, "projectClass", descriptor)
        BaseDevice.__init__(self, serverId, classId, deviceId, ifexists)

        actual = manager.getDevice(deviceId)
        actual.signalStatusChanged.connect(self.onStatusChanged)
        self.onStatusChanged(actual, actual.status, actual.error)


    def onStatusChanged(self, conf, status, error):
        """ this method gets the status of the corresponding real device,
        and finds out the gory details for this project device """
        self.error = error

        if manager.Manager().systemHash is None:
            self.status = "offline"
            return

        if status == "offline":
            try:
                attrs = manager.Manager().systemHash[
                    "server.{}".format(self.serverId), ...]
            except KeyError:
                self.status = "noserver"
            else:
                if self.classId not in attrs.get("deviceClasses", []):
                    self.status = "noplugin"
                else:
                    self.status = "offline"
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
        realDevice = manager.getDevice(self.id)
        realDevice.addVisible()


    def removeVisible(self):
        realDevice = manager.getDevice(self.id)
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
            device.initConfig = self.toHash()


    def checkDeviceSchema(self):
        """
        The device schema of an online device group is checked.
        """
        if self.descriptorRequested is True:
            return
        
        # Get device configurations
        for device in self.devices:
            conf = manager.getDevice(device.id)
            
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
            actual = manager.getDevice(d.id)
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
            realDevice = manager.getDevice(device.id)
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
    signalConfigurationAdded = pyqtSignal(str, object)
    #signalResourceAdded = pytqtSignal()
    signalMacroAdded = pyqtSignal(object)
    signalMacroChanged = pyqtSignal(object)
    
    signalRemoveObject = pyqtSignal(object)

    def __init__(self, filename):
        super(GuiProject, self).__init__(filename)

        # List of Scene
        self.scenes = []
        self.modules = { } # the modules in the macro context
        
        # States whether the project was changed or not
        self.isModified = False


    def setModified(self, isModified):
        """
        The project was modified and the associated modelview needs to be updated.
        
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
        Project.addDevice(self, device)
        self.signalDeviceAdded.emit(device)
        self.setupDeviceToProject(device)


    def insertDevice(self, index, device):
        Project.insertDevice(self, index, device)
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
            actual = manager.getDevice(device.id)
            actual.signalStatusChanged.connect(deviceGroup.onStatusChanged)
            deviceGroup.onStatusChanged(actual, actual.status, actual.error)


    def addDeviceGroup(self, deviceGroup):
        Project.addDeviceGroup(self, deviceGroup)
        self.signalDeviceGroupAdded.emit(deviceGroup)
        self.setupDeviceGroupToProject(deviceGroup)


    def insertDeviceGroup(self, index, deviceGroup):
        Project.insertDeviceGroup(self, index, deviceGroup)
        self.signalDeviceGroupInserted.emit(index, deviceGroup)
        self.setupDeviceGroupToProject(deviceGroup)


    def newDeviceGroup(self, groupId, serverId, classId, ifexists,
                             prefix, start, end):
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
            actual = manager.getDevice(device.id)
            actual.signalStatusChanged.connect(deviceGroup.onStatusChanged)
            deviceGroup.onStatusChanged(actual, actual.status, actual.error)
            
            deviceGroup.addDevice(device)
        
        return deviceGroup


    def addScene(self, scene):
        self.scenes.append(scene)
        self.signalSceneAdded.emit(scene)
        self.setModified(True)


    def addConfiguration(self, deviceId, configuration):
        Project.addConfiguration(self, deviceId, configuration)
        self.signalConfigurationAdded.emit(deviceId, configuration)
        self.setModified(True)


    def addMacro(self, macro):
        self.macros[macro.name] = macro
        self.signalMacroAdded.emit(macro)
        self.setModified(True)


    def addResource(self, category, data):
        #self.signalResourceAdded.emit(category, data)
        self.setModified(True)
        return Project.addResource(self, category, data)


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


    def parse(self, projectConfig, zf):
        super(GuiProject, self).parse(projectConfig, zf)
        for s in projectConfig[self.SCENES_KEY]:
            scene = Scene(self, s["filename"])
            data = zf.read("{}/{}".format(self.SCENES_KEY, s["filename"]))
            scene.fromXml(data)
            self.addScene(scene)


        for k in projectConfig.get(self.MACROS_KEY, []):
            macro = Macro(self, k)
            self.addMacro(macro)

        self.setModified(False)


    def zip(self, filename=None):
        """
        This method saves this project as a zip file.
        """
        projectConfig = Hash()
        exception = None

        if filename is None:
            filename = self.filename

        if os.path.exists(filename):
            file = NamedTemporaryFile(dir=os.path.dirname(filename),
                                      delete=False)
        else:
            file = filename

        with ZipFile(file, mode="w", compression=ZIP_DEFLATED) as zf:
            deviceHash = []
            for deviceObj in self.devices:
                if isinstance(deviceObj, Device):
                    zf.writestr("{}/{}".format(self.DEVICES_KEY, deviceObj.filename),
                                deviceObj.toXml())
                    
                    deviceHash.append(Hash("serverId", deviceObj.serverId,
                                           "classId", deviceObj.classId,
                                           "filename", deviceObj.filename,
                                           "ifexists", deviceObj.ifexists))
                else:
                    group = []
                    for device in deviceObj.devices:
                        # In case this device of the group was never clicked, there might
                        # be no configuration set... To prevent this, use configuration
                        # of device group
                        if deviceObj.descriptor is None:
                            device.initConfig = deviceObj.initConfig
                        else:
                            device.initConfig = deviceObj.toHash()
                        # Save configuration to XML file
                        zf.writestr("{}/{}".format(self.DEVICES_KEY, device.filename),
                                    device.toXml())
                        group.append(Hash("serverId", device.serverId,
                                          "classId", device.classId,
                                          "filename", device.filename,
                                          "ifexists", device.ifexists))
                    # Save group configuration to file
                    zf.writestr("{}/{}".format(self.DEVICES_KEY, deviceObj.filename),
                                deviceObj.toXml())
                    deviceGroupHash = Hash("group", group)
                    deviceGroupHash.setAttribute("group", "filename", deviceObj.filename)
                    deviceGroupHash.setAttribute("group", "serverId", deviceObj.serverId)
                    deviceGroupHash.setAttribute("group", "classId", deviceObj.classId)
                    deviceGroupHash.setAttribute("group", "ifexists", deviceObj.ifexists)
                    
                    deviceHash.append(deviceGroupHash)
            projectConfig[self.DEVICES_KEY] = deviceHash
            
            for scene in self.scenes:
                name = "{}/{}".format(self.SCENES_KEY, scene.filename)
                try:
                    zf.writestr(name, scene.toXml())
                except Exception as e:
                    if file is not self.filename:
                        with ZipFile(self.filename, 'r') as zin:
                            zf.writestr(name, zin.read(name))
                    if exception is None:
                        exception = e
            projectConfig[self.SCENES_KEY] = [Hash("filename", scene.filename)
                                              for scene in self.scenes]

            configs = Hash()
            for deviceId, configList in self.configurations.items():
                configs[deviceId] = [Hash("filename", c.filename)
                                     for c in configList]
                for c in configList:
                    zf.writestr("{}/{}".format(self.CONFIGURATIONS_KEY,
                                               c.filename), c.toXml())
            projectConfig[self.CONFIGURATIONS_KEY] = configs

            resources = Hash()
            if file is not self.filename:
                with ZipFile(self.filename, "r") as zin:
                    for k, v in self.resources.items():
                        for fn in v:
                            f = "resources/{}/{}".format(k, fn)
                            zf.writestr(f, zin.read(f))
                        resources[k] = StringList(v)
            projectConfig["resources"] = resources

            macros = Hash()
            for m in self.macros.values():
                f = "macros/{}.py".format(m.name)
                if m.editor is None:
                    with ZipFile(self.filename, "r") as zin:
                        t = zin.read(f)
                else:
                    t = m.editor.edit.toPlainText()
                zf.writestr(f, t)
                try:
                    zf.writestr(cache_from_source(f),
                                marshal.dumps(compile(t, f, "exec")))
                except SyntaxError:
                    pass
                macros[m.name] = f
            projectConfig[self.MACROS_KEY] = macros

            # Create folder structure and save content
            projectConfig = Hash(self.PROJECT_KEY, projectConfig)
            projectConfig[self.PROJECT_KEY, "version"] = self.version
            projectConfig[self.PROJECT_KEY, "uuid"] = self.uuid
            zf.writestr("{}.xml".format(Project.PROJECT_KEY),
                        XMLWriter().write(projectConfig))

        if file is not filename:
            file.close()
            os.remove(filename)
            os.rename(file.name, filename)

        if exception is not None:
            raise exception
        
        self.setModified(False)


    def _instantiateDevice(self, device):
        if device.isOnline():
            if device.ifexists == "ignore":
                return
            elif device.ifexists == "restart":
                # This might take a while...
                manager.Manager().shutdownDevice(device.id, False)

        if device.descriptor is None:
            config = device.initConfig
        else:
            config = device.toHash()

        manager.Manager().initDevice(device.serverId, device.classId, device.id,
                                     config)


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
                    gd.initConfig = device.toHash()
                self._instantiateDevice(gd)


    def instantiateAll(self):
        """
        This function instantiates all project devices.
        """
        for d in self.devices:
            self.instantiate(d)


    def _shutdownDevice(self, device, showConfirm):
        manager.Manager().shutdownDevice(device.id, showConfirm)


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


class Macro(object):
    def __init__(self, project, name):
        self.project = project
        self.name = name
        self.macros = {}
        self.editor = None
        self.instanceId = "Macro-{}-{}".format(self.project.name, self.name)
        self.instances = []


    def run(self):
        if self.editor is None:
            code = self.load()
        else:
            code = self.editor.edit.toPlainText()
        h = Hash("code", code,
                 "project", self.project.name,
                 "module", self.name)
        network.onInitDevice("macroServer", "MetaMacro", self.instanceId, h)


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
