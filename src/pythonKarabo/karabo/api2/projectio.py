import os
from tempfile import NamedTemporaryFile
from zipfile import ZipFile, ZIP_DEFLATED

from .hash import Hash, StringList
from .project import Project


def read_project(path, factories, instance=None):
    """ Create a new project instance from a config Hash and open zipfile.
    """
    if instance is not None:
        proj = instance
    else:
        proj = Project(path)

    with ZipFile(path, "r") as zf:
        projectHash = _read_xml_hash(zf, "{}.xml".format(Project.PROJECT_KEY))

        proj.version = projectHash[Project.PROJECT_KEY, "version"]
        proj.uuid = projectHash[Project.PROJECT_KEY, ...].get("uuid", proj.uuid)

        projectConfig = projectHash[Project.PROJECT_KEY]
        dev_groups, devices = _read_devices(zf, projectConfig, factories)
        for grp in dev_groups:
            grp.project = proj  # FIXME: This should be a weak reference
            proj.addDeviceGroup(grp)
        for dev in devices:
            proj.addDevice(dev)

        _read_configurations(zf, projectConfig, proj, factories)

        macros = _read_macros(zf, projectConfig, proj, factories)
        for m in macros:
            proj.addMacro(m)

        monitors = _read_monitors(zf, projectConfig, factories)
        for mon in monitors:
            proj.addMonitor(mon)

        scenes = _read_scenes(zf, projectConfig, proj, factories)
        for s in scenes:
            proj.addScene(s)

        proj.resources = {k: set(v) for k, v in
                          projectConfig[Project.RESOURCES_KEY].items()}


def write_project(proj, path=None):
    """ Write a project out to a zipfile at \path.
    """
    projectConfig = Hash()
    exception = None

    if path is None:
        path = proj.filename

    if os.path.exists(path):
        file = NamedTemporaryFile(dir=os.path.dirname(path), delete=False)
    else:
        file = path

    isNewFile = file is not proj.filename

    with ZipFile(file, mode="w", compression=ZIP_DEFLATED) as zf:
        deviceHashes = _write_devices(zf, proj.devices)
        projectConfig[Project.DEVICES_KEY] = deviceHashes

        sceneHashes, exception = _write_scenes(zf, proj, proj.scenes,
                                               isNewFile)
        projectConfig[Project.SCENES_KEY] = sceneHashes

        configs = _write_configurations(zf, proj.configurations)
        projectConfig[Project.CONFIGURATIONS_KEY] = configs

        resources = _write_resources(zf, proj, proj.resources, isNewFile)
        projectConfig[Project.RESOURCES_KEY] = resources

        macros = _write_macros(zf, proj, proj.macros)
        projectConfig[Project.MACROS_KEY] = macros

        monitors, exception = _write_monitors(zf, proj, proj.monitors,
                                              isNewFile)
        projectConfig[Project.MONITORS_KEY] = monitors

        # Create folder structure and save content
        projectConfig = Hash(Project.PROJECT_KEY, projectConfig)
        projectConfig[Project.PROJECT_KEY, "version"] = proj.version
        projectConfig[Project.PROJECT_KEY, "uuid"] = proj.uuid
        zf.writestr("{}.xml".format(Project.PROJECT_KEY),
                    projectConfig.encode("XML"))

    if file is not path:
        file.close()
        os.remove(path)
        os.rename(file.name, path)

    if exception is not None:
        raise exception


def _read_configurations(zf, projectConfig, projInstance, factories):
    """ Read all the project configurations from a project zipfile.
    """
    configFactory = factories['ProjectConfiguration']

    for devId, configList in projectConfig[Project.CONFIGURATIONS_KEY].items():
        # Vector of hashes
        for c in configList:
            filename = c.get("filename")
            configuration = configFactory(projInstance, filename)
            data = zf.read("{}/{}".format(Project.CONFIGURATIONS_KEY, filename))
            configuration.fromXml(data)
            projInstance.addConfiguration(devId, configuration)


def _read_devices(zf, projectConfig, factories):
    """ Read all the devices from a project zipfile.
    """
    device_groups, devices = [], []
    deviceFactory = factories['Device']
    deviceGroupFactory = factories['DeviceGroup']

    for dev in projectConfig[Project.DEVICES_KEY]:
        group = dev.get("group")
        if group is not None:
            filename = dev.getAttribute("group", "filename")
            assert filename.endswith(".xml")
            filename = filename[:-4]

            configPath = "{}/{}.xml".format(Project.DEVICES_KEY, filename)
            grpConfig = _read_xml_hash(zf, configPath)
            for _, config in grpConfig.items():
                serverId = dev.getAttribute("group", "serverId")
                classId = dev.getAttribute("group", "classId")
                # This is currently for backporting
                if dev.hasAttribute("group", "ifexists"):
                    ifexists = dev.getAttribute("group", "ifexists")
                else:
                    ifexists = "ignore"  # Use default

                devGroup = deviceGroupFactory(filename, serverId, classId,
                                              ifexists)
                devGroup.initConfig = config
                break  # there better be only one!

            for item in group:
                serverId = item.get("serverId")
                filename = item.get("filename")
                assert filename.endswith(".xml")
                filename = filename[:-4]

                configPath = "{}/{}.xml".format(Project.DEVICES_KEY, filename)
                devConfig = _read_xml_hash(zf, configPath)
                for classId, config in devConfig.items():
                    device = deviceFactory(serverId, classId, filename,
                                           item.get("ifexists"))

                    device.initConfig = config
                    devGroup.addDevice(device)
                    break  # there better be only one!
            device_groups.append(devGroup)
        else:
            serverId = dev.get("serverId")
            filename = dev.get("filename")
            assert filename.endswith(".xml")
            filename = filename[:-4]

            configPath = "{}/{}.xml".format(Project.DEVICES_KEY, filename)
            devConfig = _read_xml_hash(zf, configPath)
            for classId, config in devConfig.items():
                device = deviceFactory(serverId, classId, filename,
                                       dev.get("ifexists"))
                device.initConfig = config
                devices.append(device)
                break  # there better be only one!

    return device_groups, devices


def _read_macros(zf, projectConfig, projInstance, factories):
    """ Read all the macros from a project zipfile.
    """
    macros = []
    macroFactory = factories['Macro']

    for k in projectConfig.get(Project.MACROS_KEY, []):
        macro = macroFactory(projInstance, k)
        macros.append(macro)

    return macros


def _read_monitors(zf, projectConfig, factories):
    """ Read all the monitors from a project zipfile.
    """
    monitors = []
    monitorFactory = factories['Monitor']

    monitorHash = projectConfig.get(Project.MONITORS_KEY)
    if monitorHash is not None:
        for m in monitorHash:
            filename = m.get("filename")
            assert filename.endswith(".xml")
            data = zf.read("{}/{}".format(Project.MONITORS_KEY, filename))
            filename = filename[:-4]
            monitor = monitorFactory(filename)
            monitor.fromXml(data)
            monitors.append(monitor)

    return monitors


def _read_scenes(zf, projectConfig, projInstance, factories):
    """ Read all the scenes from a project zipfile.
    """
    scenes = []
    sceneFactory = factories['Scene']

    for s in projectConfig[Project.SCENES_KEY]:
        scene = sceneFactory(projInstance, s["filename"])
        data = zf.read("{}/{}".format(Project.SCENES_KEY, s["filename"]))
        scene.fromXml(data)
        scenes.append(scene)

    return scenes


def _read_xml_hash(zf, path):
    """ Read a Hash object (serialized as XML) from a zipfile.
    """
    return Hash.decode(zf.read(path), 'XML')


def _write_configurations(zf, objectsHash):
    """ Write all the project configurations to a project zipfile.
    """
    configs = Hash()
    for deviceId, configList in objectsHash.items():
        configs[deviceId] = [Hash("filename", c.filename) for c in configList]
        for c in configList:
            zf.writestr("{}/{}".format(Project.CONFIGURATIONS_KEY,
                                       c.filename), c.toXml())

    return configs


def _write_devices(zf, objects):
    """ Write all the devices to a project zipfile.
    """
    deviceHashes = []
    for deviceObj in objects:
        if not hasattr(deviceObj, 'devices'):
            zf.writestr("{}/{}".format(Project.DEVICES_KEY, deviceObj.filename),
                        deviceObj.toXml())

            deviceHashes.append(Hash("serverId", deviceObj.serverId,
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
                    hsh, _ = deviceObj.toHash()  # Ignore returned attributes
                    device.initConfig = hsh
                # Save configuration to XML file
                zf.writestr("{}/{}".format(Project.DEVICES_KEY, device.filename),
                            device.toXml())
                group.append(Hash("serverId", device.serverId,
                                  "classId", device.classId,
                                  "filename", device.filename,
                                  "ifexists", device.ifexists))
            # Save group configuration to file
            zf.writestr("{}/{}".format(Project.DEVICES_KEY, deviceObj.filename),
                        deviceObj.toXml())
            deviceGroupHash = Hash("group", group)
            deviceGroupHash["group", "filename"] = deviceObj.filename
            deviceGroupHash["group", "serverId"] = deviceObj.serverId
            deviceGroupHash["group", "classId"] = deviceObj.classId
            deviceGroupHash["group", "ifexists"] = deviceObj.ifexists

            deviceHashes.append(deviceGroupHash)

    return deviceHashes


def _write_macros(zf, projInstance, objectsDict):
    """ Write all the macros to a project zipfile.
    """
    macros = Hash()

    for m in objectsDict.values():
        f = "macros/{}.py".format(m.name)
        if m.editor is None:
            with ZipFile(projInstance.filename, "r") as zin:
                t = zin.read(f)
        else:
            t = m.editor.edit.toPlainText()
        zf.writestr(f, t)
        macros[m.name] = f

    return macros


def _write_monitors(zf, projInstance, objects, isNewFile):
    """ Write all the monitors to a project zipfile.
    """
    exception = None
    monitors = []

    for monitor in objects:
        name = "{}/{}".format(Project.MONITORS_KEY, monitor.filename)
        monitors.append(Hash("filename", monitor.filename))
        try:
            zf.writestr(name, monitor.toXml())
        except Exception as e:
            if isNewFile:
                with ZipFile(projInstance.filename, 'r') as zin:
                    zf.writestr(name, zin.read(name))
            if exception is None:
                exception = e

    return monitors, exception


def _write_resources(zf, projInstance, objectsHash, isNewFile):
    """
    """
    resources = Hash()

    if isNewFile:
        with ZipFile(projInstance.filename, "r") as zin:
            for k, v in objectsHash.items():
                for fn in v:
                    f = "resources/{}/{}".format(k, fn)
                    zf.writestr(f, zin.read(f))
                resources[k] = StringList(v)

    return resources


def _write_scenes(zf, projInstance, objects, isNewFile):
    """ Write all the scenes to a project zipfile.
    """
    exception = None
    sceneHashes = []

    for scene in objects:
        name = "{}/{}".format(Project.SCENES_KEY, scene.filename)
        try:
            zf.writestr(name, scene.toXml())
        except Exception as e:
            if isNewFile:
                with ZipFile(projInstance.filename, 'r') as zin:
                    zf.writestr(name, zin.read(name))
            if exception is None:
                exception = e
        sceneHashes.append(Hash("filename", scene.filename))

    return sceneHashes, exception
