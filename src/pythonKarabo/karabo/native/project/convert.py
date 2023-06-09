# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from collections import defaultdict

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel,
    ProjectModel)
from karabo.common.scenemodel.api import BaseIconsModel, DisplayIconsetModel


def convert_old_project(old_project):
    """ Given an old Project object ``old_project``, create a ProjectModel
    object which is equivalent.
    """
    project = ProjectModel(
        simple_name=old_project.name,
        macros=old_project.macros,
        scenes=_convert_scenes(old_project, old_project.scenes),
        servers=_convert_devices(old_project.devices)
    )
    # Set initialized and modified last
    project.initialized = project.modified = True
    return project

# -----------------------------------------------------------------------------


def _convert_devices(old_devices):
    """ Convert a list of old-style device instances to a list of
    DeviceConfigurationModel instances and a list of DeviceServerModel
    instances.
    """
    dev_instances = defaultdict(list)

    for dev in old_devices:
        if hasattr(dev, 'devices'):
            continue  # Skip DeviceGroup objects

        config_model = DeviceConfigurationModel(class_id=dev.classId,
                                                configuration=dev.initConfig)
        # Set initialized and modified last
        config_model.initialized = config_model.modified = True
        instance_model = DeviceInstanceModel(
            class_id=dev.classId, instance_id=dev.filename.split('.')[0],
            configs=[config_model],
            active_config_ref=config_model.uuid)
        # Set initialized and modified last
        instance_model.initialized = instance_model.modified = True
        dev_instances[dev.serverId].append(instance_model)

    servers = [DeviceServerModel(server_id=server_id, devices=instances)
               for server_id, instances in dev_instances.items()]

    # Set initialized and modified last
    for serv in servers:
        serv.initialized = serv.modified = True

    return servers


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

    return scenes
