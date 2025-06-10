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
from collections import Counter

from traits.api import Instance, List

from karabo.common.api import walk_traits_object

from .bases import BaseProjectObjectModel
from .device import DeviceInstanceModel
from .macro import MacroModel
from .server import DeviceServerModel


def find_parent_object(model, ancestor_model, search_klass):
    """Given a project child object and a project object which is the child's
    ancestor, find the immediate parent of the child.

    :param model: A project object instance
    :param ancestor_model: A project object model which is the ancestor of
                           ``model``.
    :param search_klass: The type of parent object to look for. This can be a
                         class, type, or tuple as it is passed to `isinstance`.
    :return: A parent project object model or None
    """

    class _Visitor:
        found_parent = None
        parent_candidate = None

        def __call__(self, obj, parent):
            if isinstance(parent, search_klass):
                self.parent_candidate = parent
            if obj is model:
                if self.found_parent is not None:
                    msg = "Object {} has more than one parent!"
                    raise RuntimeError(msg.format(obj))
                self.found_parent = self.parent_candidate

    visitor = _Visitor()
    walk_traits_object(ancestor_model, visitor, pass_parent=True)
    return visitor.found_parent


def device_instance_exists(project, instance_ids):
    """Check whether the a ``project`` already has a device[s] with the given
    ``instance_ids`` and return the list of devices that are duplicated
    """
    found = []

    # Allow one or more instance ids
    if isinstance(instance_ids, str):
        instance_ids = (instance_ids,)

    def visitor(obj):
        nonlocal found
        if isinstance(obj, DeviceInstanceModel):
            if obj.instance_id in instance_ids:
                found.append(obj.instance_id)

    walk_traits_object(project, visitor)
    return found


def macro_exists(project, instance_ids):
    """Check whether the a ``project`` already has a macro[s] with the given
    ``instance_ids`` and return ``True`` or ``False``
    """
    found = False

    # Allow one or more instance ids, but macros have their simple_name checked
    # as their instance id is composed of uuid and simple_name
    if isinstance(instance_ids, str):
        instance_ids = (instance_ids,)

    def visitor(obj):
        nonlocal found
        if isinstance(obj, MacroModel):
            if obj.simple_name in instance_ids:
                found = True

    walk_traits_object(project, visitor)
    return found


def device_config_exists(project, instance_id, config_names):
    """Check whether the a ``project`` already has a device[s] configuration
    with the given ``config_names`` and return ``True`` or ``False``
    """
    found = False

    # Allow one or more device configuration names
    if isinstance(config_names, str):
        config_names = (config_names,)

    def visitor(obj):
        nonlocal found
        if (
                isinstance(obj, DeviceInstanceModel)
                and obj.instance_id == instance_id
        ):
            existing = {conf.simple_name for conf in obj.configs}
            if not existing.isdisjoint(config_names):
                found = True

    walk_traits_object(project, visitor)
    return found


def recursive_save_object(root, storage, domain):
    """Recursively save a project object by using a depth first traversal and
    saving all the modified ``BaseProjectObjectModel`` objects which are
    found in the object tree.
    """

    # XXX: Yes, this is duplicated code. It's basically yield parent first vs.
    # yield parent last. This should be generalized later in walk_traits_object
    def _is_list_of_has_traits(trait):
        if not isinstance(trait.trait_type, List):
            return False
        inner_type = trait.inner_traits[0].trait_type
        if not isinstance(inner_type, Instance):
            return False
        if not issubclass(inner_type.klass, BaseProjectObjectModel):
            return False
        return True

    def _find_iterables(obj):
        return [
            name
            for name in obj.copyable_trait_names()
            if _is_list_of_has_traits(obj.trait(name))
        ]

    def _tree_iter(obj):
        # Iteratively yield the children
        iterables = _find_iterables(obj)
        for name in iterables:
            children = getattr(obj, name)
            for child in children:
                yield from _tree_iter(child)
        # Yield the root last
        yield obj

    for leaf in _tree_iter(root):
        if leaf.modified:
            storage.store(domain, leaf.uuid, leaf)


def get_project_models(root):
    """Return all project model objects from a `root_model` in a list"""
    if root is None:
        return []

    def _iter_project_model(model):
        yield model
        for sub_project in model.subprojects:
            yield from _iter_project_model(sub_project)

    models = [model for model in _iter_project_model(root)]

    return models


def check_instance_duplicates(root):
    """Check the number of instances in `root`` and the duplicates."""

    devices = Counter()
    servers = Counter()

    def visitor(obj):
        if isinstance(obj, DeviceInstanceModel):
            devices[obj.instance_id] += 1
        elif isinstance(obj, DeviceServerModel):
            servers[obj.server_id] += 1

    walk_traits_object(root, visitor)

    instances = {"devices": len(devices), "servers": len(servers)}

    duplicates = {
        "devices": {k: v for k, v in devices.items() if v > 1},
        "servers": {k: v for k, v in servers.items() if v > 1}
    }
    ret = {"Instances": instances, "Duplicates": duplicates}

    return ret
