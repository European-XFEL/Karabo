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
from contextlib import AbstractAsyncContextManager


class DatabaseBase(AbstractAsyncContextManager):

    async def initialize(self):
        """"Initialize the async interface for the database"""

    async def get_configurations_from_device_name(
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
        raise NotImplementedError

    async def get_projects_data_from_device(self, domain: str, uuid: str):
        raise NotImplementedError

    async def save_item(
        self, domain: str, uuid: str, item_xml: str,
            overwrite: bool = False):
        raise NotImplementedError

    async def load_item(self, domain: str, items: list[dict]):
        raise NotImplementedError
