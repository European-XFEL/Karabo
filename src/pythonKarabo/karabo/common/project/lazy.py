#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 20, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from io import BytesIO

from traits.api import Instance, List

from .bases import BaseProjectObjectModel
from .const import PROJECT_OBJECT_CATEGORIES
from .device import DeviceGroupModel
from .model import ProjectModel
from .server import DeviceServerModel


class ProjectObjectReference(BaseProjectObjectModel):
    """ A project object refence that can be transformed into a proper model
    object at a later point in time.
    """


class LazyDeviceGroupModel(BaseProjectObjectModel):
    """ A device group with object references which must be later resolved.
    """
    devices = List(Instance(ProjectObjectReference))


class LazyDeviceServerModel(BaseProjectObjectModel):
    """ A device server with object references which must be later resolved.
    """
    devices = List(Instance(ProjectObjectReference))


class LazyProjectModel(BaseProjectObjectModel):
    """ A project filled with object references which must be later resolved.
    """
    devices = List(Instance(ProjectObjectReference))
    macros = List(Instance(ProjectObjectReference))
    monitors = List(Instance(ProjectObjectReference))
    scenes = List(Instance(ProjectObjectReference))
    servers = List(Instance(ProjectObjectReference))
    subprojects = List(Instance(ProjectObjectReference))


def read_lazy_object(uuid, revision, db_adapter, reader_func):
    """Read a lazily-loaded object.

    NOTE: Subprojects of ProjectModel objects will not be read recursively.
    They will remain as LazyProjectModel objects.

    :param uuid: The UUID of the object
    :param revision: The desired revision number of the object
    :param db_adapter: An object which gives access to data from the database
    :param reader_func: A callable which can generically read bytestreams and
                        generate project model objects
    :return: a non-lazy model object
    """
    data = db_adapter.retrieve(uuid, revision)
    lazy_object = reader_func(BytesIO(data))
    industrious_object = _get_normal_object(lazy_object)

    for trait in _get_lazy_traits(lazy_object):
        lazy_children = getattr(lazy_object, trait)
        industrious_children = []
        for child in lazy_children:
            data = db_adapter.retrieve(child.uuid, child.version)
            industrious_children.append(reader_func(BytesIO(data)))
        setattr(industrious_object, trait, industrious_children)

    return industrious_object

# -----------------------------------------------------------------------------


def _get_lazy_traits(lazy_object):
    klass_map = {
        LazyDeviceGroupModel: ('devices',),
        LazyDeviceServerModel: ('devices',),
        LazyProjectModel: PROJECT_OBJECT_CATEGORIES,
    }
    return klass_map.get(lazy_object.__class__, ())


def _get_normal_object(lazy_object):
    klass_map = {
        LazyDeviceGroupModel: DeviceGroupModel,
        LazyDeviceServerModel: DeviceServerModel,
        LazyProjectModel: ProjectModel,
    }
    klass = klass_map.get(lazy_object.__class__)
    if klass is None:
        return lazy_object
    return klass(uuid=lazy_object.uuid, version=lazy_object.version)
