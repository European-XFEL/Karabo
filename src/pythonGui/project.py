from __future__ import unicode_literals
#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on April 7, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the project related datastructure.
"""

__all__ = ["Project", "ProjectConfiguration", "Category"]


from configuration import Configuration
from scene import Scene
from karabo.hash import Hash, XMLParser, XMLWriter
from karabo.hashtypes import StringList
import manager

from PyQt4.QtCore import pyqtSignal, QDir, QFileInfo, QObject
#from PyQt4.QtGui import QMessageBox

import hashlib
import os.path
from tempfile import NamedTemporaryFile
import urllib2
import urlparse
from zipfile import ZipFile, ZIP_DEFLATED


class Project(QObject):

    DEVICES_LABEL = "Devices"
    SCENES_LABEL = "Scenes"
    MACROS_LABEL = "Macros"
    MONITORS_LABEL = "Monitors"
    RESOURCES_LABEL = "Resources"
    CONFIGURATIONS_LABEL = "Configurations"

    PROJECT_KEY = "project"
    DEVICES_KEY = "devices"
    SCENES_KEY = "scenes"
    MACROS_KEY = "macros"
    MONITORS_KEY = "monitors"
    RESOURCES_KEY = "resources"
    CONFIGURATIONS_KEY = "configurations"
    
    PROJECT_SUFFIX = "krb"


    def __init__(self, filename):
        super(Project, self).__init__()

        self.version = 1
        self.filename = filename

        # List of Configuration
        self.devices = []
        # List of Scene
        self.scenes = []
        # Map for {deviceId, [ProjectConfiguration]}
        self.configurations = dict()
        self.macros = []
        self.resources = { }
        self.monitors = []


    @property
    def name(self):
        r = os.path.basename(self.filename)
        if r.endswith(".krb"):
            return r[:-4]
        else:
            return r


    def addDevice(self, device):
        self.devices.append(device)


    def addScene(self, scene):
        self.scenes.append(scene)


    def addConfiguration(self, deviceId, configuration):
        if deviceId in self.configurations:
            self.configurations[deviceId].append(configuration)
        else:
            self.configurations[deviceId] = [configuration]


    def remove(self, object):
        """
        The \object should be removed from this project.
        """
        if isinstance(object, Configuration):
            self.devices.remove(object)
        elif isinstance(object, Scene):
            self.scenes.remove(object)


    def unzip(self):
        with ZipFile(self.filename, "r") as zf:
            data = zf.read("{}.xml".format(self.PROJECT_KEY))
            projectConfig = XMLParser().read(data)

            self.version = projectConfig[self.PROJECT_KEY, "version"]

            projectConfig = projectConfig[self.PROJECT_KEY]
            for d in projectConfig[self.DEVICES_KEY]:
                filename = d.get("filename")
                data = zf.read("{}/{}".format(self.DEVICES_KEY, filename))
                assert filename.endswith(".xml")
                filename = filename[:-4]

                for classId, config in XMLParser().read(data).iteritems():
                    device = Device(filename, classId, d.get("ifexists"), config)
                    break # there better be only one!
                self.addDevice(device)
            for s in projectConfig[self.SCENES_KEY]:
                scene = Scene(self, s["filename"])
                data = zf.read("{}/{}".format(self.SCENES_KEY, s["filename"]))
                scene.fromXml(data)
                self.addScene(scene)
            for deviceId, configList in projectConfig[
                                self.CONFIGURATIONS_KEY].iteritems():
                # Vector of hashes
                for c in configList:
                    filename = c.get("filename")
                    configuration = ProjectConfiguration(self, filename)
                    data = zf.read("{}/{}".format(self.CONFIGURATIONS_KEY,
                                                  filename))
                    configuration.fromXml(data)
                    self.addConfiguration(deviceId, configuration)
            self.resources = {k: v for k, v in
                              projectConfig["resources"].iteritems()}


    def zip(self):
        """
        This method saves this project as a zip file.
        """
        projectConfig = Hash()

        if os.path.exists(self.filename):
            file = NamedTemporaryFile(dir=os.path.dirname(self.filename),
                                      delete=False)
        else:
            file = self.filename

        with ZipFile(file, mode="w", compression=ZIP_DEFLATED) as zf:
            for device in self.devices:
                zf.writestr("{}/{}".format(self.DEVICES_KEY, device.filename),
                            device.toXml())
            projectConfig[self.DEVICES_KEY] = [
                Hash("classId", device.classId, "filename", device.filename,
                     "ifexists", device.ifexists) for device in self.devices]

            for scene in self.scenes:
                zf.writestr("{}/{}".format(self.SCENES_KEY, scene.filename),
                            scene.toXml())
            projectConfig[self.SCENES_KEY] = [Hash("filename", scene.filename)
                                              for scene in self.scenes]

            configs = Hash()
            for deviceId, configList in self.configurations.iteritems():
                configs[deviceId] = [Hash("filename", c.filename)
                                     for c in configList]
                for c in configList:
                    zf.writestr("{}/{}".format(self.CONFIGURATIONS_KEY,
                                               c.filename), c.toXml())
            projectConfig[self.CONFIGURATIONS_KEY] = configs

            resources = Hash()
            if file is not self.filename:
                with ZipFile(self.filename, "r") as zin:
                    for k, v in self.resources.iteritems():
                        for fn in v:
                            f = "resources/{}/{}".format(k, fn)
                            zf.writestr(f, zin.read(f))
                        resources[k] = v
            projectConfig["resources"] = resources

            # Create folder structure and save content
            projectConfig = Hash(self.PROJECT_KEY, projectConfig)
            projectConfig[self.PROJECT_KEY, "version"] = self.version
            zf.writestr("{}.xml".format(Project.PROJECT_KEY),
                        XMLWriter().write(projectConfig))

        if file is not self.filename:
            file.close()
            os.remove(self.filename)
            os.rename(file.name, self.filename)


    def addResource(self, category, data):
        """add the data into the resources of given category

        this returns a URL under which the resource can be opened again"""
        with ZipFile(self.filename, mode="a", compression=ZIP_DEFLATED) as zf:
            digest = hashlib.sha1(data).hexdigest()
            zf.writestr("resources/{}/{}".format(category, digest), data)
        self.resources.setdefault(category, StringList()).append(digest)
        return "project:resources/{}/{}".format(category, digest)


    def getURL(self, url):
        """retrieve the URL and return its content

        This method retrieves the content for the URL, where the URL might
        have been generated by addResource, or point to a local file or some
        HTTP site."""
        u = urlparse.urlparse(url)
        if u.scheme == "project":
            with ZipFile(self.filename, mode="r") as zf:
                return zf.read(u.path)
        else:
            return urllib2.urlopen(url).read()


class Device(Configuration):

    def __init__(self, path, classId, ifexists, config, descriptor=None):
        super(Device, self).__init__(path, "projectClass", descriptor)

        self.filename = "{}.xml".format(path)
        self.classId = classId
        self.ifexists = ifexists # restart, ignore
        # Needed in case the descriptor is not set yet
        self.futureConfig = config
        # Merge futureConfig, if descriptor is not None
        self.mergeFutureConfig()

        actual = manager.getDevice(config["deviceId"])
        actual.statusChanged.connect(self.onStatusChanged)
        self.onStatusChanged(actual, actual.status)


    def mergeFutureConfig(self):
        """
        This function merges the \self.futureConfig into the Configuration.
        This is only possible, if the descriptor has been set before.
        """
        if self.descriptor is None: return

        # Set default values for configuration
        self.setDefault()
        self.fromHash(self.futureConfig)


    def onNewDescriptor(self, conf):
        if self.descriptor is not None:
            self.redummy()
        self.descriptor = conf.descriptor
        self.mergeFutureConfig()
        manager.Manager().onShowConfiguration(self)


    def fromXml(self, xmlString):
        """
        This function loads the corresponding XML file of this configuration.
        """
        self.fromHash(XMLParser().read(xmlString))


    def toXml(self):
        """
        This function returns the configurations' XML file as a string.
        """
        config = self.toHash()
        assert "deviceId" in config
        return XMLWriter().write(Hash(self.classId, self.toHash()))


    def onStatusChanged(self, conf, status):
        """ this method gets the status of the corresponding real device,
        and finds out the gory details for this project device """
        self.error = conf.error

        if manager.Manager().systemHash is None:
            self.status = "offline"
            return

        if status == "offline":
            try:
                attrs = manager.Manager().systemHash[
                    "server.{}".format(self.futureConfig["serverId"]), ...]
            except KeyError:
                self.status = "noserver"
            else:
                if self.classId not in attrs.get("deviceClasses", []):
                    self.status = "noplugin"
                else:
                    self.status = "offline"
        else:
            if (conf.classId == self.classId and
                    conf.serverId == self.futureConfig.get("serverId")):
                self.status = status
            else:
                self.status = "incompatible"


    def isOnline(self):
        return self.status not in (
            "offline", "noplugin", "noserver", "incompatible")


class ProjectConfiguration(object):

    def __init__(self, project, name, hash=None):
        super(ProjectConfiguration, self).__init__()
        
        # Reference to the project this scene belongs to
        self.project = project

        self.filename = name
        fi = QFileInfo(self.filename)
        if len(fi.suffix()) < 1:
            self.filename = "{}.xml".format(self.filename)
        
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


class Category(object):
    """
    This class represents a project category and is only used to have an object
    for the view items.
    """
    def __init__(self, displayName):
        super(Category, self).__init__()
        
        self.displayName = displayName

