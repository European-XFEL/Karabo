#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 28, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from karabo_gui.project.topo_listener import SystemTopologyListener
from .device import DeviceInstanceModelItem, destroy_device_shadow
from .macro import MacroModelItem, MacroInstanceItem
from .scene import SceneModelItem
from .server import DeviceServerModelItem
from .project import ProjectModelItem
from .project_groups import ProjectSubgroupItem


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
                                   child_destroy=destroy_device_shadow)
    model.on_trait_change(shadow.items_assigned, 'devices')
    model.on_trait_change(shadow.items_mutated, 'devices_items')
    for device in model.devices:
        child = shadow.child_create(model=device)
        shadow.children.append(child)

    # Attach the topology listener
    callback = shadow.system_topology_callback
    shadow.topo_listener = SystemTopologyListener(callback)

    return shadow


def destroy_device_server_model_shadow(shadow_model):
    """Destroys a DeviceServerModelItem and its associated children by removing
    all previously added Traits notification handlers.
    """
    model = shadow_model.model
    model.on_trait_change(shadow_model.items_assigned, 'devices', remove=True)
    model.on_trait_change(shadow_model.items_mutated, 'devices_items',
                          remove=True)

    # Detach the topology listener
    shadow_model.topo_listener = None


def create_macro_model_shadow(model):
    """Creates a MacroModelItem to shadow a MacroModel instance for the
    purpose of interfacing with a Qt item model.

    Traits notification handlers are attached to the passed ``model`` object
    and must be detached later with a call to
    ``destroy_macro_model_shadow``
    """
    shadow = MacroModelItem(model=model,
                            child_create=MacroInstanceItem,
                            child_destroy=lambda x: None)
    model.on_trait_change(shadow.items_assigned, 'instances')
    model.on_trait_change(shadow.items_mutated, 'instances_items')

    for inst in model.instances:
        child = shadow.child_create(model=model, instance_id=inst)
        shadow.children.append(child)

    # Attach the topology listener
    callback = shadow.system_topology_callback
    shadow.topo_listener = SystemTopologyListener(callback)

    return shadow


def destroy_macro_model_shadow(shadow_model):
    """Destroys a MacroModelItem and its associated children by removing
    all previously added Traits notification handlers.
    """
    model = shadow_model.model
    model.on_trait_change(shadow_model.items_assigned, 'instances',
                          remove=True)
    model.on_trait_change(shadow_model.items_mutated, 'instances_items',
                          remove=True)

    # Detach the topology listener
    shadow_model.topo_listener = None


def create_project_model_shadow(model=None):
    """Creates a ProjectModelItem and its associated children to shadow a
    ProjectModel instance for the purpose of interfacing with a Qt item model.

    Traits notification handlers are attached to the passed in ``model`` object
    and must be detached later with a call to ``destroy_project_model_shadow``
    """
    details = (
        # (list name, group label, item create, item destroy)
        ('macros', 'Macros', create_macro_model_shadow,
         destroy_macro_model_shadow),
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
        model.on_trait_change(child.items_assigned, name)
        model.on_trait_change(child.items_mutated, name + '_items')

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
        model.on_trait_change(child.items_assigned, name, remove=True)
        model.on_trait_change(child.items_mutated, name + '_items',
                              remove=True)
        # Recurse!
        if child.child_create is create_project_model_shadow:
            for subchild in child.children:
                destroy_project_model_shadow(subchild)
