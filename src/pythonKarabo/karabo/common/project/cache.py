#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 21, 2016
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
import os
import os.path as op
from sys import platform
from xml.etree.ElementTree import fromstring


class MemCacheWrapper:
    """In-memory storage for project objects which forwards to another object
    on cache misses.
    """

    def __init__(self, data, miss_handler):
        self._data = data
        self._miss_handler = miss_handler

    def flush(self):
        """Satisfy the project storage interface."""

    def store(self, domain, uuid, data):
        """Satisfy the project storage interface."""
        domain_data = self._data.setdefault(domain, {})
        domain_data[uuid] = data

    def retrieve(self, domain, uuid, existing=None):
        """Read an object."""
        domain_data = self._data.get(domain)
        if domain_data is None:
            return self._miss_handler.retrieve(domain, uuid, existing=existing)

        data = domain_data.get(uuid)
        if data is None:
            return self._miss_handler.retrieve(domain, uuid, existing=existing)

        return data


class ProjectDBCache:
    """Local storage for objects retrieved from the project database"""

    def __init__(self, dirpath):
        self.dirpath = dirpath

    def flush(self):
        """Flush any pending reads or writes.

        For this class, this is a no-op. In the GUI ProjectDatabaseConnection
        it makes sense to buffer reads and writes.
        """

    def store(self, domain, uuid, data):
        """Add an object to the cache"""
        path = self._generate_filepath(domain, uuid)

        domain_dir = op.dirname(path)
        if not op.exists(domain_dir):
            os.mkdir(domain_dir)

        with open(path, mode="wb") as fp:
            fp.write(data.encode("utf-8"))

    def retrieve(self, domain, uuid, existing=None):
        """Read an object from the cache."""
        path = self._generate_filepath(domain, uuid)
        if not op.exists(path):
            return None

        with open(path, mode="rb") as fp:
            return fp.read().decode("utf-8")

    def get_available_domains(self):
        """Return a list of strings including available domains"""
        avail_domains = []
        for domain in os.listdir(self.dirpath):
            avail_domains.append(domain)
        return avail_domains

    def get_available_project_data(self, domain, obj_type):
        """Return list of Hashes including available project data of all
        objects of ``obj_type`` in cache

        :param domain: A string which describes the domain to search
        :param obj_type: A string which describes the object type
        :return: A list of Hashes for the given ``obj_type`` containing:
                     - 'uuid' - The unique ID of the Project
                     - 'simple_name' - The name for displaying
                     - 'item_type' - Should be project in that case
        """
        domain_dir = op.join(self.dirpath, domain)
        if not op.exists(domain_dir):
            return []

        data = []
        for uuid in os.listdir(domain_dir):
            xml = self.retrieve(domain, uuid)
            root = fromstring(xml)
            root_type = root.attrib.get("item_type")
            if root_type == obj_type:
                attrs = root.attrib
                simple_name = attrs.get("simple_name")
                is_trashed = attrs.get("is_trashed", "").lower() == "true"
                date = attrs.get("date", "")
                data.append(
                    {"uuid": uuid,
                     "simple_name": simple_name,
                     "is_trashed": is_trashed,
                     "date": date})

        return data

    def _generate_filepath(self, domain, uuid):
        return op.join(self.dirpath, domain, uuid)


def get_user_cache():
    """Return a cache residing in the user's home directory."""
    if platform.startswith("win"):
        karabo_dir = op.join(os.environ["APPDATA"], "karabo")
    else:
        karabo_dir = op.join(os.environ["HOME"], ".karabo")
    cache_dir = op.join(karabo_dir, "project_db_cache")
    os.makedirs(cache_dir, exist_ok=True)
    return ProjectDBCache(cache_dir)
