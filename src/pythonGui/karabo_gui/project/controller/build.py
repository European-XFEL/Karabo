#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 28, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from karabo_gui.project.topo_listener import SystemTopologyListener
from .device import DeviceInstanceController
from .device_config import DeviceConfigurationController
from .macro import MacroController, MacroInstanceController
from .scene import SceneController
from .server import DeviceServerController
from .project import ProjectController
from .project_groups import ProjectSubgroupController


def create_device_server_controller(model):
    """Creates a DeviceServerController and its associated children to control
    a DeviceServerModel instance for the purpose of interfacing with a Qt item
    model.

    Traits notification handlers are attached to the passed ``model`` object
    and must be detached later with a call to
    ``destroy_device_server_controller``
    """
    controller = DeviceServerController(
        model=model,
        child_create=create_device_instance_controller,
        child_destroy=destroy_device_instance_controller
    )
    model.on_trait_change(controller.items_assigned, 'devices')
    model.on_trait_change(controller.items_mutated, 'devices_items')
    for device in model.devices:
        child = controller.child_create(model=device)
        controller.children.append(child)

    # Attach the topology listener
    callback = controller.system_topology_callback
    controller.topo_listener = SystemTopologyListener(callback)

    return controller


def destroy_device_server_controller(controller):
    """Destroys a DeviceServerController and its associated children by
    removing all previously added Traits notification handlers.
    """
    model = controller.model
    model.on_trait_change(controller.items_assigned, 'devices', remove=True)
    model.on_trait_change(controller.items_mutated, 'devices_items',
                          remove=True)

    # Detach the topology listener
    controller.topo_listener = None


def create_device_instance_controller(model):
    """Creates a DeviceInstanceController and its associated children to
    control a DeviceInstanceModel instance for the purpose of interfacing with
    a Qt item model.

    Traits notification handlers are attached to the passed ``model`` object
    and must be detached later with a call to
    ``destroy_device_instance_controller``
    """
    controller = DeviceInstanceController(
        model=model,
        child_create=DeviceConfigurationController,
        child_destroy=lambda x: None
    )
    model.on_trait_change(controller.items_assigned, 'configs')
    model.on_trait_change(controller.items_mutated, 'configs_items')
    for conf in model.configs:
        child = controller.child_create(model=conf)
        controller.children.append(child)

    return controller


def destroy_device_instance_controller(controller):
    """Destroys a DeviceInstanceController and its associated children by
    removing all previously added Traits notification handlers.
    """
    model = controller.model
    model.on_trait_change(controller.items_assigned, 'configs', remove=True)
    model.on_trait_change(controller.items_mutated, 'configs_items',
                          remove=True)


def create_macro_controller(model):
    """Creates a MacroController to control a MacroModel instance for the
    purpose of interfacing with a Qt item model.

    Traits notification handlers are attached to the passed ``model`` object
    and must be detached later with a call to ``destroy_macro_controller``
    """
    controller = MacroController(model=model,
                                 child_create=MacroInstanceController,
                                 child_destroy=lambda x: None)
    model.on_trait_change(controller.items_assigned, 'instances')
    model.on_trait_change(controller.items_mutated, 'instances_items')

    for inst in model.instances:
        child = controller.child_create(model=model, instance_id=inst)
        controller.children.append(child)

    # Attach the topology listener
    callback = controller.system_topology_callback
    controller.topo_listener = SystemTopologyListener(callback)

    return controller


def destroy_macro_controller(controller):
    """Destroys a MacroController and its associated children by removing
    all previously added Traits notification handlers.
    """
    model = controller.model
    model.on_trait_change(controller.items_assigned, 'instances', remove=True)
    model.on_trait_change(controller.items_mutated, 'instances_items',
                          remove=True)

    # Detach the topology listener
    controller.topo_listener = None


def create_project_controller(model=None):
    """Creates a ProjectController and its associated children to control a
    ProjectModel instance for the purpose of interfacing with a Qt item model.

    Traits notification handlers are attached to the passed in ``model`` object
    and must be detached later with a call to ``destroy_project_controller``
    """
    details = (
        # (list name, group label, item create, item destroy)
        ('macros', 'Macros', create_macro_controller,
         destroy_macro_controller),
        ('scenes', 'Scenes', SceneController, lambda x: None),
        ('servers', 'Device Servers', create_device_server_controller,
         destroy_device_server_controller),
        ('subprojects', 'Subprojects', create_project_controller,
         destroy_project_controller),
    )

    controller = ProjectController(model=model)
    for name, label, creator, destroyer in details:
        child = ProjectSubgroupController(model=model,
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
        controller.children.append(child)

    return controller


def destroy_project_controller(controller):
    """Destroys a ProjectController and its associated children by removing
    all previously added Traits notification handlers.
    """
    model = controller.model
    for child in controller.children:
        name = child.trait_name
        model.on_trait_change(child.items_assigned, name, remove=True)
        model.on_trait_change(child.items_mutated, name + '_items',
                              remove=True)
        # Recurse!
        if child.child_create is create_project_controller:
            for subchild in child.children:
                destroy_project_controller(subchild)
