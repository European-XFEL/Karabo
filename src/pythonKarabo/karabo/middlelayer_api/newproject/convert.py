from karabo.common.project.api import DeviceConfigurationModel, ProjectModel
from karabo.common.scenemodel.api import BaseIconsModel, DisplayIconsetModel


def convert_old_project(old_project):
    """ Given an old Project object ``old_project``, create a ProjectModel
    object which is equivalent.
    """
    project = ProjectModel(
        devices=_convert_devices(old_project.devices),
        macros=old_project.macros,
        scenes=_convert_scenes(old_project, old_project.scenes),
    )

    return project

# -----------------------------------------------------------------------------


def _convert_devices(devices):
    """ Convert a list of old-style device instances to
    DeviceConfigurationModel instances
    """
    ret_devices = []
    for dev in devices:
        if hasattr(dev, 'devices'):
            continue  # Skip DeviceGroup objects

        model = DeviceConfigurationModel(
            server_id=dev.serverId,
            class_id=dev.classId,
            instance_id=dev.filename,
            if_exists=dev.ifexists,
            configuration=dev.initConfig
        )
        ret_devices.append(model)
    return ret_devices


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
