#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 20, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from io import StringIO

from karabo.common.api import walk_traits_object
from .bases import BaseProjectObjectModel


def read_lazy_object(domain, uuid, revision, db_iface, reader, existing=None):
    """Read a project data model object which contains children which are
    actually references to other objects in the database. All children will
    be loaded recursively.

    :param uuid: The UUID of the object
    :param revision: The desired revision number of the object
    :param db_iface: An object which gives access to data from the database
    :param reader: A callable which can generically read bytestreams and
                   generate project model objects
    :param existing: An optional pre-existing, partially loaded object to
                     update with the data which is loaded.
    :return: a project model object
    """
    requested_objs = set()
    return _read_lazy_object_r(domain, uuid, revision, db_iface, reader,
                               existing, requested_objs)


def _read_lazy_object_r(domain, uuid, revision, db_iface, reader, existing,
                        requested_objs):
    """Recursive version of read_lazy_object
    """
    collected = []

    def visitor(child):
        nonlocal collected
        if isinstance(child, BaseProjectObjectModel):
            if (child.uuid, child.revision) not in requested_objs:
                collected.append(child)

    data = db_iface.retrieve(domain, uuid, revision, existing=existing)
    requested_objs.add((uuid, revision))

    if data:
        obj = reader(StringIO(data), existing=existing)

        walk_traits_object(obj, visitor)
        for child in collected:
            if child is obj:
                continue
            _read_lazy_object_r(domain, child.uuid, child.revision, db_iface,
                                reader, child, requested_objs)

        return obj
    else:
        return existing
