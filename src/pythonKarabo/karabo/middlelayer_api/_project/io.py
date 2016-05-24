from contextlib import contextmanager
from os import remove, rename
from os.path import dirname, exists
from tempfile import NamedTemporaryFile
from zipfile import ZipFile, ZIP_DEFLATED

from karabo.middlelayer_api.hash import Hash, StringList
from .configuration import ProjectConfigurationData
from .constants import (DEVICES_KEY, SCENES_KEY, CONFIGURATIONS_KEY, MACROS_KEY,
                        MONITORS_KEY, PROJECT_KEY, RESOURCES_KEY)
from .device import DeviceData, DeviceGroupData
from .macro import MacroData
from .model import ProjectData
from .monitor import MonitorData
from .scene import SceneData


def read_project(path):
    """ Create a new project instance from a config Hash and open zipfile.
    """
    with ZipFile(path, "r") as zf:
        filename = "{}.xml".format(PROJECT_KEY)
        projectHash = _read_xml_hash(zf, filename)
        config = projectHash[PROJECT_KEY, ...]
        proj = ProjectData(path, config=config)

        projectConfig = projectHash[PROJECT_KEY]
        _read_configurations(zf, projectConfig, proj)

        devices = _read_devices(zf, projectConfig)
        for dev in devices:
            proj.addDevice(dev)

        macros = _read_macros(zf, projectConfig)
        for m in macros:
            proj.addMacro(m)

        monitors = _read_monitors(zf, projectConfig)
        for mon in monitors:
            proj.addMonitor(mon)

        scenes = _read_scenes(zf, projectConfig)
        for s in scenes:
            proj.addScene(s)

        proj.resources = {k: set(v) for k, v in
                          projectConfig[RESOURCES_KEY].items()}

    return proj


def write_project(proj, path=None):
    """ Write a project out to a zipfile at \path.
    """
    projectConfig = Hash()

    if path is None:
        path = proj.filename

    if not exists(proj.filename) and path == proj.filename:
        # Make sure the resource copying code doesn't barf.
        with ZipFile(path, 'w'):
            pass

    @contextmanager
    def _safeDestination(target):
        with NamedTemporaryFile(dir=dirname(target), delete=False) as file:
            yield file.name

            # Move to the target path when everything is safely written
            if exists(target):
                remove(target)
            rename(file.name, target)

    with _safeDestination(path) as fn:
        with ZipFile(fn, mode="w", compression=ZIP_DEFLATED) as zf:
            configs = _write_configurations(zf, proj.configurations)
            deviceHashes = _write_devices(zf, proj.devices)
            macros = _write_macros(zf, proj.macros.values())
            monitors = _write_monitors(zf, proj.monitors.values())
            resources = _write_resources(zf, proj, proj.resources)
            sceneHashes = _write_scenes(zf, proj.scenes.values())

            # XXX: This order is meant to match the old ordering of the
            # project writing code. Since Hash objects are OrderedDicts, order
            # matters. (Because we want to produce identical output for
            # round-trip project data)
            projectConfig[DEVICES_KEY] = deviceHashes
            projectConfig[SCENES_KEY] = sceneHashes
            projectConfig[CONFIGURATIONS_KEY] = configs
            projectConfig[RESOURCES_KEY] = resources
            projectConfig[MACROS_KEY] = macros
            projectConfig[MONITORS_KEY] = monitors

            # Create folder structure and save content
            projectConfig = Hash(PROJECT_KEY, projectConfig)
            projectConfig[PROJECT_KEY, "version"] = proj.version
            projectConfig[PROJECT_KEY, "uuid"] = proj.uuid
            zf.writestr("{}.xml".format(PROJECT_KEY),
                        projectConfig.encode("XML"))


def _add_xml_ext(name):
    """ Most of the filenames need .xml on the end, so...
    """
    return name + '.xml'


def _read_configurations(zf, projectConfig, projInstance):
    """ Read all the project configurations from a project zipfile.
    """
    factory = ProjectConfigurationData.deserialize
    configItemsView = projectConfig[CONFIGURATIONS_KEY].items()
    basePath = "{0}/{{}}".format(CONFIGURATIONS_KEY)

    for devId, configList in configItemsView:
        # Vector of hashes
        for c in configList:
            name = c.get("filename")
            config = factory(name, zf.read(basePath.format(name)))
            projInstance.addConfiguration(devId, config)


def _read_devices(zf, projectConfig):
    """ Read all the devices from a project zipfile.
    """
    devices = []
    basePath = "{0}/{{}}.xml".format(DEVICES_KEY)

    for dev in projectConfig[DEVICES_KEY]:
        group = dev.get("group")
        if group is not None:
            name = dev["group", "filename"]
            assert name.endswith(".xml")
            name = name[:-4]

            configPath = basePath.format(name)
            grpConfig = _read_xml_hash(zf, configPath)
            for _, config in grpConfig.items():
                serverId = dev["group", "serverId"]
                classId = dev["group", "classId"]
                # This is currently for backporting
                if dev.hasAttribute("group", "ifexists"):
                    ifexists = dev["group", "ifexists"]
                else:
                    ifexists = "ignore"  # Use default

                devGroup = DeviceGroupData(serverId, classId, name, ifexists,
                                           config=config)
                break  # there better be only one!

            for item in group:
                serverId = item.get("serverId")
                name = item.get("filename")
                assert name.endswith(".xml")
                name = name[:-4]

                configPath = basePath.format(name)
                ifexists = item.get("ifexists")
                devConfig = _read_xml_hash(zf, configPath)
                for classId, config in devConfig.items():
                    device = DeviceData(serverId, classId, name, ifexists,
                                        config=config)
                    devGroup.addDevice(device)
                    break  # there better be only one!

            devices.append(devGroup)
        else:
            serverId = dev.get("serverId")
            ifexists = dev.get("ifexists")
            name = dev.get("filename")

            assert name.endswith(".xml")
            name = name[:-4]

            configPath = basePath.format(name)
            devConfig = _read_xml_hash(zf, configPath)
            for classId, config in devConfig.items():
                device = DeviceData(serverId, classId, name, ifexists,
                                    config=config)
                devices.append(device)
                break  # there better be only one!

    return devices


def _read_macros(zf, projectConfig):
    """ Read all the macros from a project zipfile.
    """
    macros = []
    factory = MacroData.deserialize
    basePath = "{0}/{{}}.py".format(MACROS_KEY)

    for k in projectConfig.get(MACROS_KEY, []):
        macro = factory(k, zf.read(basePath.format(k)))
        macros.append(macro)

    return macros


def _read_monitors(zf, projectConfig):
    """ Read all the monitors from a project zipfile.
    """
    monitors = []
    factory = MonitorData.deserialize
    basePath = '{0}/{{}}'.format(MONITORS_KEY)

    for m in projectConfig.get(MONITORS_KEY, []):
        name = m['filename']
        assert name.endswith('.xml')

        monitor = factory(name[:-4], zf.read(basePath.format(name)))
        monitors.append(monitor)

    return monitors


def _read_scenes(zf, projectConfig):
    """ Read all the scenes from a project zipfile.
    """
    scenes = []
    factory = SceneData.deserialize
    basePath = '{0}/{{}}'.format(SCENES_KEY)

    for s in projectConfig.get(SCENES_KEY, []):
        name = s['filename']
        scene = factory(name, zf.read(basePath.format(name)))
        scenes.append(scene)

    return scenes


def _read_xml_hash(zf, path):
    """ Read a Hash object (serialized as XML) from a zipfile.
    """
    return Hash.decode(zf.read(path), 'XML')


def _write_configurations(zf, objectsDict):
    """ Write all the project configurations to a project zipfile.
    """
    configs = Hash()
    basePath = '{0}/{{}}'.format(CONFIGURATIONS_KEY)

    for deviceId, configList in objectsDict.items():
        configs[deviceId] = [Hash('filename', c.name) for c in configList]
        for c in configList:
            zf.writestr(basePath.format(c.name), c.serialize())

    return configs


def _write_devices(zf, objects):
    """ Write all the devices to a project zipfile.
    """
    deviceHashes = []
    basePath = '{0}/{{}}'.format(DEVICES_KEY)

    def _write(dev, hashList):
        filename = _add_xml_ext(dev.name)
        zf.writestr(basePath.format(filename), dev.serialize())
        hashList.append(Hash('serverId', dev.serverId, 'classId', dev.classId,
                             'filename', filename, 'ifexists', dev.ifexists))

    for deviceObj in objects:
        if not hasattr(deviceObj, 'devices'):
            _write(deviceObj, deviceHashes)
        else:
            group = []
            for device in deviceObj.devices:
                if device.config.empty():
                    device.config = deviceObj.config
                _write(device, group)

            # Save group configuration to file
            filename = _add_xml_ext(deviceObj.name)
            zf.writestr(basePath.format(filename), deviceObj.serialize())

            deviceGroupHash = Hash('group', group)
            deviceGroupHash['group', 'filename'] = filename
            deviceGroupHash['group', 'serverId'] = deviceObj.serverId
            deviceGroupHash['group', 'classId'] = deviceObj.classId
            deviceGroupHash['group', 'ifexists'] = deviceObj.ifexists

            deviceHashes.append(deviceGroupHash)

    return deviceHashes


def _write_macros(zf, objects):
    """ Write all the macros to a project zipfile.
    """
    macros = Hash()
    basePath = '{0}/{{}}.py'.format(MACROS_KEY)

    for m in objects:
        path = basePath.format(m.name)
        zf.writestr(path, m.serialize())
        macros[m.name] = path

    return macros


def _write_monitors(zf, objects):
    """ Write all the monitors to a project zipfile.
    """
    monitors = []
    basePath = '{0}/{{}}'.format(MONITORS_KEY)

    for monitor in objects:
        filename = _add_xml_ext(monitor.name)
        path = basePath.format(filename)
        zf.writestr(path, monitor.serialize())
        monitors.append(Hash("filename", filename))

    return monitors


def _write_resources(zf, projInstance, objectsHash):
    """ Copy the resources from an existing project file into a new file.
    """
    resources = Hash()

    with ZipFile(projInstance.filename, "r") as zin:
        for k, v in objectsHash.items():
            for fn in v:
                f = "resources/{}/{}".format(k, fn)
                zf.writestr(f, zin.read(f))
            resources[k] = StringList(v)

    return resources


def _write_scenes(zf, objects):
    """ Write all the scenes to a project zipfile.
    """
    sceneHashes = []
    basePath = '{0}/{{}}'.format(SCENES_KEY)

    for scene in objects:
        name = basePath.format(scene.name)
        zf.writestr(name, scene.serialize())
        sceneHashes.append(Hash("filename", scene.name))

    return sceneHashes
