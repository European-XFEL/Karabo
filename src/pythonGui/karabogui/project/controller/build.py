#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 28, 2016
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from karabogui.project.topo_listener import SystemTopologyListener
from karabogui.singletons.api import get_topology

from .device import DeviceInstanceController
from .macro import MacroController, MacroInstanceController
from .project import ProjectController
from .project_groups import ProjectSubgroupController
from .scene import SceneController
from .server import DeviceServerController
from .subproject import SubprojectController


def create_device_server_controller(model=None, parent=None, _qt_model=None):
    """Creates a DeviceServerController and its associated children to control
    a DeviceServerModel instance for the purpose of interfacing with a Qt item
    model.

    Traits notification handlers are attached to the passed ``model`` object
    and must be detached later with a call to
    ``destroy_device_server_controller``
    """
    controller = DeviceServerController(
        model=model, parent=parent, _qt_model=_qt_model,
        child_create=create_device_instance_controller,
        child_destroy=destroy_device_instance_controller
    )
    model.on_trait_change(controller.items_assigned, 'devices')
    model.on_trait_change(controller.items_mutated, 'devices_items')
    for device in model.devices:
        child = controller.child_create(model=device, parent=controller,
                                        _qt_model=_qt_model)
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


def create_device_instance_controller(model=None, parent=None, _qt_model=None):
    """Creates a DeviceInstanceController and its associated children to
    control a DeviceInstanceModel instance for the purpose of interfacing with
    a Qt item model.

    Traits notification handlers are attached to the passed ``model`` object
    and must be detached later with a call to
    ``destroy_device_instance_controller``
    """
    controller = DeviceInstanceController(
        model=model, parent=parent, _qt_model=_qt_model)

    return controller


def destroy_device_instance_controller(controller):
    """Destroys a DeviceInstanceController and its associated children by
    removing all previously added Traits notification handlers.
    """
    model = controller.model
    # Remove project device references
    get_topology().delete_project_device(model.instance_id)
    controller.project_device = None


def create_macro_controller(model=None, parent=None, _qt_model=None):
    """Creates a MacroController to control a MacroModel instance for the
    purpose of interfacing with a Qt item model.

    Traits notification handlers are attached to the passed ``model`` object
    and must be detached later with a call to ``destroy_macro_controller``
    """
    controller = MacroController(model=model, parent=parent,
                                 _qt_model=_qt_model,
                                 child_create=MacroInstanceController,
                                 child_destroy=lambda x: None)
    model.on_trait_change(controller.items_assigned, 'instances')
    model.on_trait_change(controller.items_mutated, 'instances_items')

    for inst in model.instances:
        child = controller.child_create(model=model, parent=controller,
                                        _qt_model=_qt_model,
                                        instance_id=inst)
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


def create_project_controller(model=None, parent=None, _qt_model=None):
    """Creates a ProjectController and its associated children to control a
    ProjectModel instance for the purpose of interfacing with a Qt item model.

    Traits notification handlers are attached to the passed in ``model`` object
    and must be detached later with a call to ``destroy_project_controller``
    """
    details = (
        # (list name, group label, group class, item create, item destroy)
        ('macros', 'Macros', ProjectSubgroupController,
         create_macro_controller, destroy_macro_controller),
        ('scenes', 'Scenes', ProjectSubgroupController,
         SceneController, lambda x: None),
        ('servers', 'Device Servers', ProjectSubgroupController,
         create_device_server_controller, destroy_device_server_controller),
        ('subprojects', 'Subprojects', SubprojectController,
         create_project_controller, destroy_project_controller),
    )

    controller = ProjectController(model=model, parent=parent,
                                   _qt_model=_qt_model)
    for name, label, klass, creator, destroyer in details:
        child = klass(model=model, parent=controller, _qt_model=_qt_model,
                      group_name=label, trait_name=name, child_create=creator,
                      child_destroy=destroyer)
        model.on_trait_change(child.items_assigned, name)
        model.on_trait_change(child.items_mutated, name + '_items')

        subchildren = getattr(model, name)
        subitemchildren = [creator(model=submodel, parent=child,
                                   _qt_model=_qt_model)
                           for submodel in subchildren]
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
        for subchild in child.children:
            child.child_destroy(subchild)
