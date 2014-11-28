
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
from karabo.hash import Hash, XMLParser, XMLWriter
from karabo.hashtypes import StringList
from karabo.project import Project, BaseDevice, BaseDeviceGroup
import karabo
import manager
from finders import MacroContext

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


    def isOnline(self):
        raise NotImplementedError("BaseConfiguration.isOnline")



class Device(BaseDevice, BaseConfiguration):
    signalDeviceModified = pyqtSignal(bool)
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


    def isOnline(self):
        return self.status not in (
            "offline", "noplugin", "noserver", "incompatible")


    def addVisible(self):
        print()
        print("Device.addVisible", self)
        realDevice = manager.getDevice(self.id)
        realDevice.addVisible()


    def removeVisible(self):
        print()
        print("Device.removeVisible")
        realDevice = manager.getDevice(self.id)
        realDevice.removeVisible()


class DeviceGroup(BaseDeviceGroup, BaseConfiguration):


    def __init__(self, id="", type="deviceGroupClass"):
        BaseConfiguration.__init__(self, id, type)
        BaseDeviceGroup.__init__(self, id)
        
        # If all device are online there is another deviceGroup to represent this
        self.instance = None


    def createInstance(self):
        """
        The online instance of the deviceGroup is created once and returned.
        """
        if self.instance is not None:
            return self.instance

        self.instance = DeviceGroup(self.id, "deviceGroup")
        self.instance.devices = self.devices
        self.instance.project = self.project
        
        self.instance.serverId = self.serverId
        self.instance.classId = self.classId

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


    def _connectDevices(self, descriptor, groupBox):
        """
        This recursive function connects all boxes of this device group to the
        corresponding boxes of the various devices.
        This allows the correct update of all box values once a device group
        configuration is changed.
        """
        for k, v in descriptor.dict.items():
            childBox = getattr(groupBox, k, None)
            if childBox is None:
                continue

            for device in self.devices:
                if self.isOnline():
                    # Connect online devices
                    conf = manager.getDevice(device.id)
                    deviceBox = conf.getBox(childBox.path)
                    if deviceBox is None:
                        continue
                    childBox.signalUserChanged.connect(deviceBox.signalUserChanged)
                else:
                    # Connect project devices
                    deviceBox = device.getBox(childBox.path)
                    if deviceBox is None:
                        continue
                    childBox.signalUpdateComponent.connect(deviceBox.onUpdateValue)
            
            if isinstance(v, Schema):
                self._connectDevices(v, childBox.boxvalue)


    def checkDeviceSchema(self):
        if self.descriptorRequested is True:
            return
        
        # Get device configurations
        for device in self.devices:
            conf = manager.getDevice(device.id)
            
            conf.signalNewDescriptor.connect(self.onNewDescriptor)
            if conf.descriptor is not None:
                self.onNewDescriptor(conf)
            # Do this only for first device - it is expected that the deviceSchema
            # is equal for all group devices
            break

        self.descriptorRequested = True


    def onNewDescriptor(self, conf):
        BaseConfiguration.onNewDescriptor(self, conf)
        # Recursively connect deviceGroup boxes to boxes of devices
        self._connectDevices(self.descriptor, self.boxvalue)


    def isOnline(self):
        return not [d for d in self.devices \
             if d.status in ("offline", "noplugin", "noserver", "incompatible")]


    def addVisible(self):
        print()
        print("DeviceGroup.addVisible", self.devices)
        for device in self.devices:
            realDevice = manager.getDevice(device.id)
            realDevice.addVisible()


    def removeVisible(self):
        print()
        print("DeviceGroup.removeVisible")
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
        device.signalDeviceModified.connect(self.setModified)


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


    def addDeviceGroup(self, deviceGroup):
        Project.addDeviceGroup(self, deviceGroup)
        self.signalDeviceGroupAdded.emit(deviceGroup)
        self.setModified(True)
        for device in deviceGroup.devices:
            device.signalDeviceNeedsUpdate.connect(deviceGroup.onUpdateDevice)


    def newDeviceGroup(self, groupId, serverId, classId, deviceId, ifexists,
                             prefix, start, end):
        deviceGroup = DeviceGroup(groupId)
        # Set server and class id for descriptor request
        deviceGroup.serverId = serverId
        deviceGroup.classId = classId
        
        Project.addDeviceGroup(self, deviceGroup)
        
        for index in range(start, end):
            id = "{}{}{}".format(deviceId, prefix, index)
            device = Device(serverId, classId, id, ifexists)
            device.signalDeviceNeedsUpdate.connect(deviceGroup.onUpdateDevice)
            deviceGroup.addDevice(device)
        
        self.signalDeviceGroupAdded.emit(deviceGroup)
        self.setModified(True)
        
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


        for k in projectConfig[self.MACROS_KEY]:
            macro = Macro(self, k)
            self.addMacro(macro)

        with MacroContext(self):
            try:
                for m in self.macros.values():
                    m.run()
            except Exception as e:
                QMessageBox.warning(None, "Macro Error",
                        "error excuting macro {}:\n{}".format(m.name, e))

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
                    deviceGroupHash.setAttribute("group", "id", deviceObj.id)
                    deviceGroupHash.setAttribute("group", "filename", deviceObj.filename)
                    deviceGroupHash.setAttribute("group", "serverId", deviceObj.serverId)
                    deviceGroupHash.setAttribute("group", "classId", deviceObj.classId)
                    
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
        self._instantiateDevice(device)


    def instantiateAll(self):
        """
        This function instantiates all project devices.
        """
        for d in self.devices:
            self._instantiateDevice(d)


    def shutdown(self, device, showConfirm=True):
        """
        This function shuts down \device.
        """
        manager.Manager().shutdownDevice(device.id, showConfirm)


    def shutdownAll(self):
        """
        This function shuts down all project devices.
        """
        for d in self.devices:
            manager.Manager().shutdownDevice(d.id, False)


class Macro(object):
    def __init__(self, project, name):
        self.project = project
        self.name = name
        self.module = None
        self.macros = { }
        self.editor = None


    def run(self):
        try:
            if self.module is None:
                self.module = import_module("macros." + self.name)
            else:
                self.module = reload(self.module)
        except:
            self.module = None
            raise
        self.macros = {k: v for k, v in self.module.__dict__.items()
                       if isinstance(v, type) and
                          issubclass(v, karabo.Macro) and
                          v is not karabo.Macro}
        for k, v in self.macros.items():
            v.instance = Configuration(k, "macro", v.getSchema())
            v.instance.setDefault()
            v.instance.status = "alive"
        
        self.project.signalMacroChanged.emit(self)


    def load(self):
        with ZipFile(self.project.filename, "r") as zf:
            return zf.read("macros/{}.py".format(self.name)).decode("utf8")


    def exec_module(self, module):
        fn = "macros/{}.py".format(self.name)
        if self.editor is None:
            with ZipFile(self.project.filename, "r") as zf:
                try:
                    code = marshal.loads(zf.read(cache_from_source(fn)))
                except (KeyError, EOFError, ValueError, TypeError):
                    exec(compile(zf.read(fn), fn, "exec"), module.__dict__)
                else:
                    exec(code, module.__dict__)
        else:
            exec(compile(self.editor.edit.toPlainText(), fn, "exec"),
                 module.__dict__)


class Category(object):
    """
    This class represents a project category and is only used to have an object
    for the view items.
    """
    def __init__(self, displayName):
        super(Category, self).__init__()
        
        self.displayName = displayName
