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
from time import gmtime, strftime

from .util import ProjectDBError, make_str_if_needed, make_xml_if_needed

DATE_FORMAT = '%Y-%m-%d %H:%M:%S'


class HandleABC(ABC):
    """Database Handle ABC

    it enforces only the basic "load/save" functionality.
    Queries are left to implementation in the derived class.
    """
    @abstractmethod
    def hasCollection(self, path):
        raise NotImplementedError

    @abstractmethod
    def removeCollection(self, path):
        raise NotImplementedError

    @abstractmethod
    def createCollection(self, path):
        raise NotImplementedError

    @abstractmethod
    def hasDocument(self, path):
        raise NotImplementedError

    @abstractmethod
    def load(self, data, path):
        raise NotImplementedError

    @abstractmethod
    def getDoc(self, name):
        raise NotImplementedError

    @abstractmethod
    def getDocument(self, name):
        raise NotImplementedError


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
        `self.dbhandle` object attribute in the `__enter__` conte
        """

    def path(self, domain, uuid):
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

    def domain_exists(self, domain):
        """
        Checks if a given domain exists.
        :param domain: the domain to check
        :return: True if it exists, false otherwise
        """
        path = f"{self.root}/{domain}"
        return self.dbhandle.hasCollection(path)

    def add_domain(self, domain):
        """
        Adds a domain to the project database. A domain is a top-level
        collection located directly underneath the self.root collection. When
        created the following collections will be added to the domain:
        projects, scenes, macros, configs, device_servers, and resources.

        :param domain: the name of the domain to be created
        :return:None
        :raises: ProjectDBError on Handle failure,
                 or RuntimeError if domain creation failed otherwise
        """
        path = f"{self.root}/{domain}"
        success = self.dbhandle.createCollection(path)

        if not success:
            raise RuntimeError(f"Failed to create domain at {path}")

    def sanitize_database(self, domain):
        """Optional Method

        Migrates a DB to the latest sane configuration"""

    def get_configurations_from_device_name(self, domain, instance_id):
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

    def get_configurations_from_device_name_part(self, domain, device_id_part):
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

    def get_projects_from_device(self, domain, uuid):
        """
        Returns the projects which contain a device instance with a given uuid

        To be implemented in the derived class

        :param domain: DB domain
        :param uuid: the uuid of the device instance from the database
        :return: a set containing project names
        """
        raise NotImplementedError

    def get_projects_data_from_device(self, domain, uuid):
        raise NotImplementedError

    def get_projects_with_conf(self, domain, device_id):
        """
        Returns a dict with projects and active configurations from a device
        name.

        :param domain: DB domain
        :param device_id: the device to return the information for.
        :return: a dict:
            {"project name": configuration uuid,
             ...}
        """
        configs = self.get_configurations_from_device_name(domain,
                                                           device_id)
        projects = dict()
        for config in configs:
            instance_id = config["instanceid"]
            for project in self.get_projects_from_device(domain,
                                                         instance_id):
                projects[project] = config["configid"]
        return projects

    def get_projects_with_device(self, domain, device_id_part):
        """
        Returns a dict with data about projects that contain active
        configurations for a given device.

        :param domain: DB domain
        :param device_id_part: part of name of devices for which project data
                               must be returned.
        :return: a list of dicts:
            [{"projectname": name of project,
              "date": last modification timestamp for the project,
              "uuid": uuid of projecti
              "devices": list of ids of prj devices with the given part}, ...]
        """
        configs = self.get_configurations_from_device_name_part(domain,
                                                                device_id_part)
        projects = []
        for config in configs:
            device_uuid = config["device_uuid"]
            device_id = config["device_id"]
            for prj in self.get_projects_data_from_device(domain,
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

    def save_item(self, domain, uuid, item_xml, overwrite=False):
        """
        Saves a item xml file into the domain. It will
        create a new entry if the item does not exist yet, or create a new
        version of the item if it does exist. In case of a versioning
        conflict the update will fail and the most current version of the
        item file is returned.

        The root node of the xml should contain a `item_type` entry identifying
        the type of the item as one of the following:

        'projects', 'scenes', 'macros', 'device_configs', 'device_servers'

        If domain does not exist it is created given a user has appropriate
        access rights.

        :param domain: the domain under which this item is to be stored
        :param uuid: the item's uuid
        :param item_xml: the xml containing the item information
        :param overwrite: defaults to False. If set to True versioning
                          information is removed prior to database injection,
                          allowing to overwrite in case of versioning
                          conflicts.

        :return: (True, dict) if successful, (false, dict) with dict
                 representing the item's versioning information. In case of
                 version conflict this will contain the information of the
                 *newest existing* entry in the database. If saving was
                 successful (no conflict), it will contain the updated
                 versioning information after saving the item.


        :raises: ProjectDBError on Handle Failure.
            RuntimeError if item saving failed otherwise.
            AttributeError if a non-supported type is passed
        """

        # create domain if necessary
        if not self.domain_exists(domain):
            self.add_domain(domain)

        # Extract some information
        try:
            # NOTE: The client might send us garbage
            item_tree = make_xml_if_needed(item_xml)
        except ValueError:
            msg = f'XML parse error for item "{uuid}"'
            raise ProjectDBError(msg)

        if 'user' not in item_tree.attrib:
            item_tree.attrib['user'] = 'Karabo User'
        if not item_tree.attrib.get('date'):
            item_tree.attrib['date'] = strftime(DATE_FORMAT, gmtime())
        else:
            modified, reason = self._check_for_modification(
                domain, uuid, item_tree.attrib['date'])
            if modified:
                message = "The <b>{}</b> item <b>{}</b> could not be saved: " \
                          "{}".format(item_tree.attrib.get('item_type', ''),
                                      item_tree.attrib.get('simple_name', ''),
                                      reason)
                raise ProjectDBError(message)
            # Update time stamp
            item_tree.attrib['date'] = strftime(DATE_FORMAT, gmtime())

        # XXX: Add a revision/alias to keep old code from blowing up
        item_tree.attrib['revision'] = '0'
        item_tree.attrib['alias'] = 'default'

        item_xml = make_str_if_needed(item_tree)
        path = self.path(domain, uuid)

        if self.dbhandle.hasDocument(path) and not overwrite:
            raise ProjectDBError("Versioning conflict. Document exists!")
        # NOTE: The underlying HTTP code needs bytes here...
        success = self.dbhandle.load(item_xml.encode('utf8'), path)
        if not success:
            raise ProjectDBError("Saving item failed!")

        meta = {}
        meta['domain'] = domain
        meta['uuid'] = uuid
        meta['date'] = item_tree.attrib['date']
        return meta
