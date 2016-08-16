
#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 10, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents project related datastructures.
"""

from enum import Enum
import hashlib
import os.path
import urllib.request, urllib.error, urllib.parse
import urllib.parse
from uuid import uuid4
from zipfile import ZipFile, ZIP_DEFLATED

from .hash import XMLParser, XMLWriter
from karabo.common.scenemodel.api import read_scene


class ProjectAccess(Enum):
    """ These states describes the access to a project. """
    CLOUD = 0 # read and write access
    LOCAL = 1 # read and write access
    CLOUD_READONLY = 2 # readonly access


class Project(object):

    DEVICES_LABEL = "Devices"
    SCENES_LABEL = "Scenes"
    CONFIGURATIONS_LABEL = "Configurations"
    MACROS_LABEL = "Macros"
    MONITORS_LABEL = "Monitors"
    RESOURCES_LABEL = "Resources"

    PROJECT_KEY = "project"
    DEVICES_KEY = "devices"
    SCENES_KEY = "scenes"
    CONFIGURATIONS_KEY = "configurations"
    MACROS_KEY = "macros"
    MONITORS_KEY = "monitors"
    RESOURCES_KEY = "resources"

    PROJECT_SUFFIX = "krb"

    def __init__(self, filename):
        super(Project, self).__init__()

        self.version = 1
        self.filename = filename
        self.uuid = str(uuid4())
        self.access = ProjectAccess.LOCAL # LOCAL, CLOUD, CLOUD_READONLY

        # List of devices
        self.devices = []
        # Map for {deviceId, [ProjectConfiguration]}
        self.configurations = {}
        self.macros = {}
        self.monitors = []
        self.resources = {}
        self.scenes = []

        self.monitorFilename = ""
        self.monitorInterval = 0
        self.isMonitoring = False

    @property
    def name(self):
        """
        This function returns the name of the project excluding the suffix.
        """
        r = os.path.basename(self.filename)
        if r.endswith(".krb"):
            return r[:-4]
        else:
            return r

    @property
    def basename(self):
        """
        This function returns the name of the project including the suffix.
        """
        b = os.path.basename(self.filename)
        if not b.endswith(".krb"):
            return "{}.krb".format(b)
        else:
            return b

    def addConfiguration(self, deviceId, configuration, index=-1):
        if deviceId in self.configurations:
            if index < 1:
                self.configurations[deviceId].append(configuration)
            else:
                self.configurations[deviceId].insert(index, configuration)
        else:
            self.configurations[deviceId] = [configuration]

    def removeConfiguration(self, deviceId, configuration):
        """
        Remove the ProjectConfiguration from the configurations dictionary.

        If the list of device configurations just has the given configuration,
        the complete entry is removed.
        
        The \index of the configuration in the list of device configurations is
        returned.
        If there was only this one configuration in list 0 is returned.
        If the \deviceId can not be found in dict -1 is returned.
        """
        if configuration in self.configurations[deviceId]:
            if len(self.configurations[deviceId]) == 1:
                del self.configurations[deviceId]
                return 0
            else:
                index = self.configurations[deviceId].index(configuration)
                self.configurations[deviceId].pop(index)
                return index
        
        return -1

    def addDevice(self, device):
        self.devices.append(device)

    def insertDevice(self, index, device):
        self.devices.insert(index, device)

    def getDevice(self, devId):
        """
        The first occurrence of the device (group) with the given \devId is
        returned.
        """
        for device in self.devices:
            if devId == device.id:
                return device
        return None

    def getDevices(self, deviceIds):
        """
        This function returns a list of all associated devices for the given
        \deviceIds.
        """
        devices = []
        for d in self.devices:
            if d.deviceId in deviceIds:
                devices.append(d)
        return devices

    def addDeviceGroup(self, deviceGroup):
        self.devices.append(deviceGroup)

    def insertDeviceGroup(self, index, deviceGroup):
        self.devices.insert(index, deviceGroup)

    def addMacro(self, macro):
        self.macros[macro.name] = macro

    def getMacro(self, name):
        """ Return the macro with the given \name
        """
        return self.macros[name]

    def removeMacro(self, name):
        """ Remove the macro with the given \name
        """
        if name in self.macros:
            del self.macros[name]

    def addMonitor(self, monitor):
        self.monitors.append(monitor)

    def getMonitor(self, name):
        """
        The first occurence of the monitor with the given \name is returned.
        """
        for monitor in self.monitors:
            if name == monitor.name:
                return monitor
        return None

    def insertMonitor(self, index, monitor):
        self.monitors.insert(index, monitor)

    def addResource(self, category, data):
        """add the data into the resources of given category

        this returns a URL under which the resource can be opened again
        """
        def _hasPath(zf, path):
            try:
                zf.getinfo(path)
                return True
            except KeyError:
                return False

        with ZipFile(self.filename, mode="a", compression=ZIP_DEFLATED) as zf:
            digest = hashlib.sha1(data).hexdigest()
            respath = "resources/{}/{}".format(category, digest)
            # Duplicate items compute the same hash. Don't write them twice.
            if not _hasPath(zf, respath):
                zf.writestr(respath, data)
        self.resources.setdefault(category, set()).add(digest)
        return "project:{}".format(respath)

    def addScene(self, scene):
        self.scenes.append(scene)

    def insertScene(self, index, scene):
        self.scenes.insert(index, scene)

    def getScene(self, title):
        """ The first occurrence of a scene with the given \title is
        returned.
        """
        for scene in self.scenes:
            if title == scene.title:
                return scene

    def getSceneNames(self):
        """ All the scene names for this project.
        """
        return [scene.title for scene in self.scenes]

    def removeScene(self, title):
        """ The first occurrence of a scene with the given \title is
        removed.
        """
        for scene in self.scenes:
            if title == scene.title:
                self.scenes.remove(scene)
                return

    def getURL(self, url):
        """retrieve the URL and return its content

        This method retrieves the content for the URL, where the URL might
        have been generated by addResource, or point to a local file or some
        HTTP site."""
        u = urllib.parse.urlparse(url)
        if u.scheme == "project":
            with ZipFile(self.filename, mode="r") as zf:
                return zf.read(u.path)
        elif u.scheme == "": # for old projects, delete later
            with open(url, mode="r") as fin:
                return fin.read()
        else:
            return urllib.request.urlopen(url).read()

    def remove(self, object):
        """
        The \object should be removed from this project.
        
        Returns \index of the object in the list.
        """
        raise NotImplementedError("Project.remove")

    def unzip(self, factories=None):
        """ Read the zip file zf. The file must already be open for reading.
        """
        # FIXME: Resolve the circular import be removing import of Project
        # FIXME: in the projectio module.
        from .projectio import read_project

        objFactories = {
            'Device': BaseDevice,
            'DeviceGroup': BaseDeviceGroup,
            'Macro': BaseMacro,
            'Monitor': Monitor,
            'ProjectConfiguration': ProjectConfiguration,
            'Scene': read_scene,
        }
        if factories is not None:
            objFactories.update(factories)

        read_project(self.filename, objFactories, instance=self)

    def zip(self, filename=None):
        """ This method saves this project as a zip file.
        """
        # FIXME: Resolve the circular import be removing import of Project
        # FIXME: in the projectio module.
        from .projectio import write_project

        write_project(self, path=filename)

    def instantiate(self, deviceIds):
        """
        This function instantiates the list of \devices.
        """
        raise NotImplementedError("Project.instantiate")

    def instantiateAll(self):
        """
        This function instantiates all project devices.
        """
        raise NotImplementedError("Project.instantiateAll")

    def shutdown(self, deviceIds):
        """
        This function shuts down the list of \devices.
        """
        raise NotImplementedError("Project.shutdown")

    def shutdownAll(self):
        """
        This function shuts down all project devices.
        """
        raise NotImplementedError("Project.shutdownAll")


class ProjectConfiguration(object):
    def __init__(self, project, name, hash=None):
        super(ProjectConfiguration, self).__init__()

        if name.endswith(".xml"):
            self.filename = name
        else:
            self.filename = "{}.xml".format(name)
        
        self.hash = hash

    def fromXml(self, xmlString):
        """
        This function loads the corresponding XML file of this configuration.
        """
        self.hash = XMLParser().read(xmlString)

    def toXml(self):
        """
        This function returns the configurations' XML file as a string.
        """
        return XMLWriter().write(self.hash)


class BaseDevice(object):
    def __init__(self, serverId, classId, deviceId, ifexists):
        assert ifexists in ("ignore", "restart")
        
        self.serverId = serverId
        self.classId = classId

        self.filename = "{}.xml".format(deviceId)
        self.ifexists = ifexists


class BaseDeviceGroup(BaseDevice):
    """
    This class represents a list of devices and is a device itself.
    """

    def __init__(self, serverId, classId, id, ifexists):
        BaseDevice.__init__(self, serverId, classId, id, ifexists)

        self.devices = []

    def addDevice(self, device):
        self.devices.append(device)


class BaseMacro(object):
    """ A simple macro object which supports file round-trips.
    """
    def __init__(self, project, name):
        self.name = name
        self.editor = None
        self.instanceId = "Macro-{}-{}".format(project.name, self.name)


class Monitor(object):
    """
    This class represents a datastructure which is needed for the later run-control.
    """

    def __init__(self, name, config=None):
        super(Monitor, self).__init__()

        self.name = name
        
        # This hash contains all necessary data like:
        # name
        # deviceId
        # deviceProperty
        # metricPrefixSymbol (optional)
        # unitSymbol
        # format
        self.config = config

    @property
    def name(self):
        """
        This function returns the name of the monitor excluding the suffix.
        """
        r = os.path.basename(self.filename)
        if r.endswith(".xml"):
            return r[:-4]
        else:
            return r

    @name.setter
    def name(self, name):
        """
        This function sets the name of the monitor excluding the suffix.
        """
        self.filename = "{}.xml".format(name)

    def fromXml(self, xmlString):
        """
        This function loads the corresponding XML file of this configuration.
        """
        self.config = XMLParser().read(xmlString)

    def toXml(self):
        """
        This function returns the configurations' XML file as a string.
        """
        return XMLWriter().write(self.config)

