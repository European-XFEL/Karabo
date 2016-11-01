#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 20, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from io import BytesIO

from traits.api import Instance, List

from .bases import BaseProjectObjectModel, ProjectObjectReference
from .const import PROJECT_OBJECT_CATEGORIES
from .model import ProjectModel


class LazyProjectModel(BaseProjectObjectModel):
    """ A project filled with object references which must be later resolved.
    """
    macros = List(Instance(ProjectObjectReference))
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
            data = db_adapter.retrieve(child.uuid, child.revision)
            industrious_children.append(reader_func(BytesIO(data)))
        setattr(industrious_object, trait, industrious_children)

    return industrious_object

# -----------------------------------------------------------------------------


def _get_lazy_traits(lazy_object):
    klass_map = {
        LazyProjectModel: PROJECT_OBJECT_CATEGORIES,
    }
    return klass_map.get(lazy_object.__class__, ())


def _get_normal_object(lazy_object):
    klass_map = {
        LazyProjectModel: ProjectModel,
    }
    klass = klass_map.get(lazy_object.__class__)
    if klass is None:
        return lazy_object
    copy_names = ('uuid', 'revision', 'simple_name', 'db_attrs')
    traits = {n: getattr(lazy_object, n) for n in copy_names}
    return klass(**traits)
