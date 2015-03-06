
#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 10, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents project related datastructures.
"""


__all__ = ["Project", "ProjectConfiguration", "BaseDevice", "BaseDeviceGroup"]

from karabo.hash import XMLParser, XMLWriter

from enum import Enum
import hashlib
import os.path
import urllib.request, urllib.error, urllib.parse
import urllib.parse
from uuid import uuid4
from zipfile import ZipFile, ZIP_DEFLATED

class ProjectAccess(Enum):
    """ These states describes the access to a project. """
    CLOUD = 0 # read and write access
    LOCAL = 1 # read and write access
    CLOUD_READONLY = 2 # readonly access


class Project(object):

    DEVICES_LABEL = "Devices"
    SCENES_LABEL = "Scenes"
    MACROS_LABEL = "Macros"
    MONITORS_LABEL = "Monitors"
    RESOURCES_LABEL = "Resources"
    CONFIGURATIONS_LABEL = "Configurations"
    MONITORS_LABEL = "Monitors"

    PROJECT_KEY = "project"
    DEVICES_KEY = "devices"
    SCENES_KEY = "scenes"
    MACROS_KEY = "macros"
    MONITORS_KEY = "monitors"
    RESOURCES_KEY = "resources"
    CONFIGURATIONS_KEY = "configurations"
    MONITORS_KEY = "monitors"
    
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
        self.configurations = dict()
        self.macros = {}
        self.resources = { }
        self.monitors = []


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


    def addDevice(self, device):
        self.devices.append(device)
        device.project = self


    def insertDevice(self, index, device):
        self.devices.insert(index, device)
        device.project = self


    def getDevice(self, id):
        """
        The first occurence of the device (group) with the given \id is returned.
        """
        for device in self.devices:
            if id == device.id:
                return device
        return None


    def addDeviceGroup(self, deviceGroup):
        self.devices.append(deviceGroup)
        deviceGroup.project = self


    def insertDeviceGroup(self, index, deviceGroup):
        self.devices.insert(index, deviceGroup)
        deviceGroup.project = self


    def addConfiguration(self, deviceId, configuration):
        if deviceId in self.configurations:
            self.configurations[deviceId].append(configuration)
        else:
            self.configurations[deviceId] = [configuration]


    def removeConfiguration(self, deviceId, configuration):
        """
        Remove the ProjectConfiguration from the configurations dictionary.
        
        If the list of device configurations just has the given configuration,
        the complete entry is removed.
        """
        if configuration in self.configurations[deviceId]:
            if len(self.configurations[deviceId]) == 1:
                del self.configurations[deviceId]
            else:
                self.configurations[deviceId].remove(configuration)


    def addMonitor(self, monitor):
        self.monitors.append(monitor)
        monitor.project = self


    def getMonitor(self, name):
        """
        The first occurence of the monitor with the given \name is returned.
        """
        for monitor in self.monitors:
            if name == monitor.name:
                return monitor
        return None


    def addResource(self, category, data):
        """add the data into the resources of given category

        this returns a URL under which the resource can be opened again"""
        with ZipFile(self.filename, mode="a", compression=ZIP_DEFLATED) as zf:
            digest = hashlib.sha1(data).hexdigest()
            zf.writestr("resources/{}/{}".format(category, digest), data)
        self.resources.setdefault(category, set()).add(digest)
        return "project:resources/{}/{}".format(category, digest)


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


    def remove(self, object):
        """
        The \object should be removed from this project.
        
        Returns \index of the object in the list.
        """
        raise NotImplementedError("Project.remove")


    def unzip(self):
        """read the zip file zf. The file must already be open for reading."""
        with ZipFile(self.filename, "r") as zf:
            data = zf.read("{}.xml".format(self.PROJECT_KEY))
            projectConfig = XMLParser().read(data)

            self.version = projectConfig[self.PROJECT_KEY, "version"]
            self.uuid = projectConfig[self.PROJECT_KEY, ...].get("uuid",
                                                                 self.uuid)

            self.parse(projectConfig[self.PROJECT_KEY], zf)


    def parse(self, projectConfig, zf):
        for d in projectConfig[self.DEVICES_KEY]:
            group = d.get("group")
            if group is not None:
                filename = d.getAttribute("group", "filename")
                data = zf.read("{}/{}".format(self.DEVICES_KEY, filename))
                assert filename.endswith(".xml")
                filename = filename[:-4]
                for _, config in XMLParser().read(data).items():
                    serverId = d.getAttribute("group", "serverId")
                    classId = d.getAttribute("group", "classId")
                    # This is currently for backporting
                    if d.hasAttribute("group", "ifexists"):
                        ifexists = d.getAttribute("group", "ifexists")
                    else:
                        ifexists = "ignore" # Use default
                    
                    deviceGroup = self.DeviceGroup(filename, serverId, classId, ifexists)
                    deviceGroup.initConfig = config
                    deviceGroup.project = self
                    break # there better be only one!
                
                for item in group:
                    serverId = item.get("serverId")
                    filename = item.get("filename")
                    data = zf.read("{}/{}".format(self.DEVICES_KEY, filename))
                    assert filename.endswith(".xml")
                    filename = filename[:-4]

                    for classId, config in XMLParser().read(data).items():
                        device = self.Device(serverId, classId, filename,
                                             item.get("ifexists"))

                        device.initConfig = config
                        deviceGroup.addDevice(device)
                        break # there better be only one!
                self.addDeviceGroup(deviceGroup)
            else:
                serverId = d.get("serverId")
                filename = d.get("filename")
                data = zf.read("{}/{}".format(self.DEVICES_KEY, filename))
                assert filename.endswith(".xml")
                filename = filename[:-4]

                for classId, config in XMLParser().read(data).items():
                    device = self.Device(serverId, classId, filename,
                                         d.get("ifexists"))
                    device.initConfig = config
                    break # there better be only one!
                self.addDevice(device)
        for deviceId, configList in projectConfig[
                            self.CONFIGURATIONS_KEY].items():
            # Vector of hashes
            for c in configList:
                filename = c.get("filename")
                configuration = ProjectConfiguration(self, filename)
                data = zf.read("{}/{}".format(self.CONFIGURATIONS_KEY,
                                              filename))
                configuration.fromXml(data)
                self.addConfiguration(deviceId, configuration)
        self.resources = {k: set(v) for k, v in
                          projectConfig["resources"].items()}


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
        
        # Reference to the project this configuration belongs to
        self.project = project

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
        
        self.project = None


class BaseDeviceGroup(BaseDevice):
    """
    This class represents a list of devices and is a device itself.
    """

    def __init__(self, serverId, classId, id, ifexists):
        BaseDevice.__init__(self, serverId, classId, id, ifexists)

        self.devices = []
        self.project = None


    def addDevice(self, device):
        self.devices.append(device)
        device.project = self.project


class Monitor(object):
    """
    This class represents a datastructure which is needed for the later run-control.
    """
    
    def __init__(self, name, config=None):
        super(Monitor, self).__init__()
        
        # Reference to the project this monitor belongs to
        self.project = None
        
        self.name = name
        
        # This hash contains all necessary data like:
        # deviceId
        # deviceProperty
        # metricPrefixSymbol (optional)
        # unitSymbol
        # format
        self.config = config


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

