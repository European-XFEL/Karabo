
from collections import OrderedDict
from enum import Enum
import hashlib
from uuid import uuid4
from zipfile import ZipFile, ZIP_DEFLATED


class ProjectAccess(Enum):
    """ These states describes the access to a project. """
    CLOUD = 0  # read and write access
    LOCAL = 1  # read and write access
    CLOUD_READONLY = 2  # readonly access


class ProjectData(object):
    def __init__(self, filename, config=None):
        super(ProjectData, self).__init__()

        if config is not None:
            self.version = config['version']
            self.uuid = config['uuid']
        else:
            self.version = 1
            self.uuid = str(uuid4())

        assert self.version == 1

        self.filename = filename
        self.access = ProjectAccess.LOCAL  # LOCAL, CLOUD, CLOUD_READONLY

        # List of devices
        self.devices = []
        # Map for {deviceId, [ProjectConfiguration]}
        self.configurations = OrderedDict()
        self.macros = OrderedDict()
        self.monitors = OrderedDict()
        self.resources = OrderedDict()
        self.scenes = OrderedDict()

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
            if devId == device.name:
                return device
        return None

    def getDevices(self, deviceIds):
        """
        This function returns a list of all associated devices for the given
        \deviceIds.
        """
        devices = []
        for d in self.devices:
            if d.name in deviceIds:
                devices.append(d)
        return devices

    def addMacro(self, macro):
        name = macro.name
        assert name not in self.macros
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
        name = monitor.name
        assert name not in self.monitors
        self.monitors[name] = monitor

    def getMonitor(self, name):
        """ The first occurence of the monitor with the given \name is returned.
        """
        return self.monitors[name]

    def removeMonitor(self, name):
        """ Remove the macro with the given \name
        """
        if name in self.monitors:
            del self.monitors[name]

    @staticmethod
    def _resourcePath(category, digest):
        return "resources/{}/{}".format(category, digest)

    def addResource(self, category, data):
        """ Add the data into the resources of given category

        This returns a name under which the resource can be opened again
        """
        def _hasPath(zf, path):
            try:
                zf.getinfo(path)
                return True
            except KeyError:
                return False

        with ZipFile(self.filename, mode="a", compression=ZIP_DEFLATED) as zf:
            digest = hashlib.sha1(data).hexdigest()
            respath = self._resourcePath(category, digest)
            if not _hasPath(zf, respath):
                zf.writestr(respath, data)

        self.resources.setdefault(category, set()).add(digest)
        return digest

    def getResource(self, category, digest):
        """ Read a resource from the project file.
        """
        assert digest in self.resources[category]
        with ZipFile(self.filename, mode="r") as zf:
            data = zf.read(self._resourcePath(category, digest))
        return data

    def addScene(self, scene):
        name = scene.name
        assert name not in self.scenes
        self.scenes[name] = scene

    def getScene(self, name):
        """ The first occurrence of a scene with the given \name is
        returned.
        """
        return self.scenes[name]

    def removeScene(self, name):
        """ The first occurrence of a scene with the given \name is
        removed.
        """
        if name in self.scenes:
            del self.scenes[name]
