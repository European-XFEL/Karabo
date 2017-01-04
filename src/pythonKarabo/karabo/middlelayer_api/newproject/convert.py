from collections import defaultdict

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    ProjectModel
)
from karabo.common.scenemodel.api import BaseIconsModel, DisplayIconsetModel


def convert_old_project(old_project):
    """ Given an old Project object ``old_project``, create a ProjectModel
    object which is equivalent.
    """
    devices, servers = _convert_devices(old_project.devices)
    project = ProjectModel(
        simple_name=old_project.name,
        macros=old_project.macros,
        scenes=_convert_scenes(old_project, old_project.scenes),
        servers=servers,
        initialized=True
    )

    return project, devices

# -----------------------------------------------------------------------------


def _convert_devices(old_devices):
    """ Convert a list of old-style device instances to a list of
    DeviceConfigurationModel instances and a list of DeviceServerModel
    instances.
    """
    dev_instances = defaultdict(list)
    devices = []

    for dev in old_devices:
        if hasattr(dev, 'devices'):
            continue  # Skip DeviceGroup objects

        config_model = DeviceConfigurationModel(class_id=dev.classId,
                                                configuration=dev.initConfig,
                                                initialized=True)
        uuid, rev = config_model.uuid, config_model.revision
        instance_model = DeviceInstanceModel(
            class_id=dev.classId, instance_id=dev.filename,
            if_exists=dev.ifexists, configs=[config_model],
            active_config_ref=(uuid, rev))
        dev_instances[dev.serverId].append(instance_model)
        devices.append(config_model)

    servers = [DeviceServerModel(server_id=server_id, devices=instances,
                                 initialized=True)
               for server_id, instances in dev_instances.items()]

    return devices, servers


def _convert_scenes(project, scenes):
    """ Fill in the icon data for a list of scenes read from an old project.
    """
    def _get_image_data(model):
        if model.data:
            return  # data trait is already set!
        url = model.image
        model.data = project.getURL(url)

    def _update_icon_model(parent_model):
        for child in parent_model.children:
            if isinstance(child, BaseIconsModel):
                for icon_data in child.values:
                    _get_image_data(icon_data)
            elif isinstance(child, DisplayIconsetModel):
                _get_image_data(child)
            else:
                if hasattr(child, "children"):
                    _update_icon_model(child)

    for model in scenes:
        # Recursively set all icon model data
        _update_icon_model(model)
        # Move the old filename to the `simple_name` trait
        model.simple_name = model.title

    return scenes
