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
from abc import ABC, abstractmethod
from contextlib import ContextDecorator


class HandleABC(ABC):
    """Database Handle ABC

    it enforces only the basic "load/save" functionality.
    Queries are left to implementation in the derived class.
    """
    @abstractmethod
    def hasCollection(self, path):
        """Abstract method for DbHandle"""
        return True

    @abstractmethod
    def removeCollection(self, path):
        """Abstract method for DbHandle"""

    @abstractmethod
    def createCollection(self, path):
        """Abstract method for DbHandle"""

    @abstractmethod
    def hasDocument(self, path):
        """Abstract method for DbHandle"""
        return True

    @abstractmethod
    def load(self, data, path):
        """Abstract method for DbHandle"""

    @abstractmethod
    def getDoc(self, name):
        """Abstract method for DbHandle"""

    @abstractmethod
    def getDocument(self, name):
        """Abstract method for DbHandle"""


class DatabaseBase(ContextDecorator):
    root = None

    def __init__(self):
        self._dbhandle = None

    @property
    def dbhandle(self):
        return self._dbhandle

    @dbhandle.setter
    def dbhandle(self, value):
        assert isinstance(value, HandleABC)
        self._dbhandle = value

    def onEnter(self):
        """ To Be implemented by subclasses

        returns an object subclass of `HandleABC` which is set to the
        `self.dbhandle` object attribute in the `__aenter__` conte
        """

    def path(self, domain: str, uuid: str):
        # XXX: Add a '_0' suffix to keep old code from wetting its pants
        return f"{self.root}/{domain}/{uuid}_0"

    def __enter__(self):
        """Obtain a database handle.

        sanity checks on the database should be implemented in this function
        """
        self.dbhandle = self.onEnter()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        Clean-up action for the ProjectDatabase context. As the database
        handle doesn't carry any state nothing needs to be done here.
        :param exc_type:
        :param exc_val:
        :param exc_tb:
        :return:
        """

    async def initialize(self):
        """"Initialize the async interface for the database"""

    async def __aenter__(self):
        """Obtain a database handle.

        sanity checks on the database should be implemented in this function
        """
        self.dbhandle = self.onEnter()
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        """
        Clean-up action for the ProjectDatabase context. As the database
        handle doesn't carry any state nothing needs to be done here.
        :param exc_type:
        :param exc_val:
        :param exc_tb:
        :return:
        """

    def get_configurations_from_device_name(
            self, domain: str, instance_id: str):
        """Returns a list of configurations for a given device

        To be implemented in the derived class

        :param domain: DB domain
        :param instance_id: instance id of the device
        :return: a list of dicts:
            [{"configid": uuid of the configuration,
              "instanceid": device instance uuid in the DB},
              ...
            ]
        """
        raise NotImplementedError

    async def get_configurations_from_device_name_part(
            self, domain: str, device_id_part: str):
        """
        Returns a list of configurations for a given device.
        :param domain: DB domain
        :param device_id_part: part of device name; search is case-insensitive.
        :return: a list of dicts:
            [{"config_id": uuid of the configuration,
              "device_uuid": device instance uuid in the DB,
              "device_id": device instance id},
              ...
            ]
        """
        raise NotImplementedError

    async def get_projects_with_conf(self, domain: str, device_id: str):
        """
        Returns a dict with projects and active configurations from a device
        name.

        :param domain: DB domain
        :param device_id: the device to return the information for.
        :return: a dict:
            {"project name": configuration uuid,
             ...}
        """
        raise NotImplementedError

    async def get_projects_from_device(self, domain: str, uuid: str):
        """
        Returns the projects which contain a device instance with a given uuid

        To be implemented in the derived class

        :param domain: DB domain
        :param uuid: the uuid of the device instance from the database
        :return: a set containing project names
        """
        raise NotImplementedError

    async def get_projects_with_device(
            self, domain: str, device_id_part: str) -> list[dict[str, any]]:
        """
        Returns a list of dictionaries with data about projects that contain
        active configurations for a given device.

        :param domain: DB domain
        :param device_id_part: part of name of devices for which project data
                               must be returned.
        :return: a list of dicts:
            [{"projectname": name of project,
              "date": last modification timestamp for the project,
              "uuid": uuid of projecti
              "devices": list of ids of prj devices with the given part}, ...]
        """
        configs = await self.get_configurations_from_device_name_part(
            domain, device_id_part)
        projects = []
        for config in configs:
            device_uuid = config["device_uuid"]
            device_id = config["device_id"]
            for prj in await self.get_projects_data_from_device(domain,
                                                                device_uuid):
                prj_in_list = next((p for p in projects
                                    if p["uuid"] == prj["uuid"]), None)
                if prj_in_list:
                    # The project is already in the resulting list due to
                    # another device_id that matched the name part; add the
                    # device_id to the 'devices' attribute of the project.
                    prj_in_list["devices"].append(device_id)
                else:
                    prj["devices"] = [device_id]
                    projects.append(prj)

        return projects

    async def get_projects_data_from_device(self, domain, uuid):
        raise NotImplementedError

    async def save_item(self, domain, uuid, item_xml, overwrite=False):
        raise NotImplementedError
