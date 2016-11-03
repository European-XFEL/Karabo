#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 20, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from io import BytesIO

from .bases import BaseProjectObjectModel
from .utils import walk_traits_object


def read_lazy_object(uuid, revision, db_adapter, reader_func, existing=None):
    """Read a project data model object which contains children which are
    actually references to other objects in the database. All children will
    be loaded recursively.

    :param uuid: The UUID of the object
    :param revision: The desired revision number of the object
    :param db_adapter: An object which gives access to data from the database
    :param reader_func: A callable which can generically read bytestreams and
                        generate project model objects
    :param existing: An optional pre-existing, partially loaded object to
                     update with the data which is loaded.
    :return: a project model object
    """
    collected = []

    def visitor(child):
        nonlocal collected
        if isinstance(child, BaseProjectObjectModel) and not child.initialized:
            collected.append(child)

    data = db_adapter.retrieve(uuid, revision)
    obj = reader_func(BytesIO(data), existing=existing)
    obj.initialized = True

    walk_traits_object(obj, visitor)
    for child in collected:
        read_lazy_object(child.uuid, child.revision, db_adapter, reader_func,
                         existing=child)

    return obj
