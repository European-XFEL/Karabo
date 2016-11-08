#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 28, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from .device import DeviceInstanceModelItem
from .macro import MacroModelItem
from .scene import SceneModelItem
from .server import DeviceServerModelItem
from .project import ProjectModelItem
from .project_groups import ProjectSubgroupItem


def create_project_model_shadow(model=None):
    """Creates a ProjectModelItem and its associated children to shadow a
    ProjectModel instance for the purpose of interfacing with a Qt item model.

    Traits notification handlers are attached to the passed in ``model`` object
    and must be detached later with a call to ``destroy_project_model_shadow``
    """
    details = (
        # (list name, group label, item create, item destroy)
        ('macros', 'Macros', MacroModelItem, lambda x: None),
        ('scenes', 'Scenes', SceneModelItem, lambda x: None),
        ('servers', 'Device Servers', create_device_server_model_shadow,
         destroy_device_server_model_shadow),
        ('subprojects', 'Subprojects', create_project_model_shadow,
         destroy_project_model_shadow),
    )

    shadow = ProjectModelItem(model=model)
    for name, label, creator, destroyer in details:
        child = ProjectSubgroupItem(model=model,
                                    group_name=label,
                                    trait_name=name,
                                    child_create=creator,
                                    child_destroy=destroyer)
        model.on_trait_change(child.item_handler, name + '_items')

        subchildren = getattr(model, name)
        subitemchildren = [creator(model=submodel) for submodel in subchildren]
        # Using extend (or append) calls _children_items_changed
        child.children.extend(subitemchildren)
        shadow.children.append(child)

    return shadow


def destroy_project_model_shadow(shadow_model):
    """Destroys a ProjectModelItem and its associated children by removing
    all previously added Traits notification handlers.
    """
    model = shadow_model.model
    for child in shadow_model.children:
        name = child.trait_name
        model.on_trait_change(child.item_handler, name + '_items',
                              remove=True)
        # Recurse!
        if child.child_create is create_project_model_shadow:
            for subchild in child.children:
                destroy_project_model_shadow(subchild)


def create_device_server_model_shadow(model):
    """Creates a DeviceServerModelItem and its associated children to shadow a
    DeviceServerModel instance for the purpose of interfacing with a Qt item
    model.

    Traits notification handlers are attached to the passed ``model`` object
    and must be detached later with a call to
    ``destroy_device_server_model_shadow``
    """
    shadow = DeviceServerModelItem(model=model,
                                   child_create=DeviceInstanceModelItem,
                                   child_destroy=lambda x: None)
    model.on_trait_change(shadow.item_handler, 'devices_items')
    for device in model.devices:
        child = shadow.child_create(model=device)
        shadow.children.append(child)
    return shadow


def destroy_device_server_model_shadow(shadow_model):
    """Destroys a DeviceServerModelItem and its associated children by removing
    all previously added Traits notification handlers.
    """
    model = shadow_model.model
    model.on_trait_change(
        shadow_model.item_handler, 'devices_items', remove=True)
