#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 20, 2016
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
#############################################################################
from io import StringIO

from karabo.common.api import walk_traits_object

from .bases import BaseProjectObjectModel


def read_lazy_object(domain, uuid, db_iface, reader, existing=None):
    """Read a project data model object which contains children which are
    actually references to other objects in the database. All children will
    be loaded recursively.

    :param uuid: The UUID of the object
    :param db_iface: An object which gives access to data from the database
    :param reader: A callable which can generically read bytestreams and
                   generate project model objects
    :param existing: An optional pre-existing, partially loaded object to
                     update with the data which is loaded.
    :return: a project model object
    """
    requested_objs = set()
    return _read_lazy_object_r(
        domain, uuid, db_iface, reader, existing, requested_objs
    )


def _read_lazy_object_r(
    domain, uuid, db_iface, reader, existing, requested_objs
):
    """Recursive version of read_lazy_object"""
    collected = []

    def visitor(child):
        nonlocal collected
        if isinstance(child, BaseProjectObjectModel):
            if child.uuid not in requested_objs:
                collected.append(child)

    data = db_iface.retrieve(domain, uuid, existing=existing)
    requested_objs.add(uuid)

    if data:
        obj = reader(StringIO(data), existing=existing)

        walk_traits_object(obj, visitor)
        for child in collected:
            if child is obj:
                continue
            _read_lazy_object_r(
                domain, child.uuid, db_iface, reader, child, requested_objs
            )

        return obj
    else:
        return existing
