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
import os
from os import path as op
from time import strptime

from lxml import etree

from karabo.project_db.bases import DatabaseBase, HandleABC
from karabo.project_db.const import ROOT_COLLECTION, ROOT_COLLECTION_TEST
from karabo.project_db.util import ProjectDBError, to_string

DATE_FORMAT = '%Y-%m-%d %H:%M:%S'


class FileHandle(HandleABC):
    def __init__(self, dir):
        self.dir = dir

    def hasCollection(self, path):
        if op.exists(path):
            return True
        else:
            return False

    def removeCollection(self, path):
        """Temporarily do, nothing

        in the long term, we should remove the path"""

    def createCollection(self, path):
        success = False
        try:
            dir_ = op.join(self.dir, path)
            os.makedirs(dir_, exist_ok=True)
            success = True
        except OSError as e:
            raise ProjectDBError(e)
        return success

    def hasDocument(self, path):
        return op.exists(op.join(self.dir, path))

    def load(self, data, path):
        success = False
        try:
            with open(path, 'wb') as f:
                f.write(data)
            success = True
        except OSError as e:
            raise ProjectDBError(e)
        return success

    def getDoc(self, name):
        return self.getDocument(name)

    def getDocument(self, name):
        path = op.join(self.dir, name)
        if not op.exists(path):
            raise ProjectDBError(f"{name} missing")
        with open(path, 'rb') as f:
            return f.read()


class ProjectDatabase(DatabaseBase):

    def __init__(self, dir, test_mode=False, init_db=False):
        """
        Create a project data base context for a given user

        :param dir: the directory where the files are saved
           relative to the '$KARABO'/var/data environment variable
        :param test_mode: defaults to False. In this case
           db_settings.root_collection will be used as the root collection,
           otherwise, root_collection_test is used.
        :param init_db: defaults to False. If True, the default collections
           will be added if missing on context entry.
        :return: a FileDatabase context
        """

        root = (ROOT_COLLECTION if not test_mode else
                ROOT_COLLECTION_TEST)
        self.root = op.join(os.environ['KARABO'], 'var', 'data', dir, root)
        self._init_db = init_db
        self._dbhandle = None

    def path(self, domain, uuid):
        return op.join(self.root, domain, f"{uuid}_0")

    def onEnter(self):
        """Initialize root folder if missing"""
        if self._init_db:
            try:
                os.makedirs(self.root, exist_ok=True)
                os.makedirs(op.join(self.root, "LOCAL"), exist_ok=True)
                self._init_db = False
            except OSError as e:
                raise ProjectDBError(e)
        return FileHandle(self.root)

    def _check_for_modification(self, domain, uuid, old_date):
        """ Check whether the item with of the given `domain` and `uuid` was
        modified

        :param domain: the domain under which this item exists
        :param uuid: the item's uuid
        :param old_date: the item's last modified time stamp
        :return A tuple stating whether the item was modified inbetween and a
                string describing the reason
        """
        # Check whether the object got modified inbetween
        path = self.path(domain, uuid)
        modified = False
        reason = ""
        if op.exists(path):
            tree = etree.parse(path)
            current_date = tree.getroot().attrib['date']
        else:
            # Item not yet existing
            return modified, reason
        current_modified = strptime(current_date, DATE_FORMAT)
        last_modified = strptime(old_date, DATE_FORMAT)
        if last_modified < current_modified:
            # Item got saved inbetween
            modified = True
            reason = "Versioning conflict! Document modified inbetween."
        return modified, reason

    def load_item(self, domain, items):
        """
        Load an item or items from `domain`
        :param domain: a domain to load from
        :param item: the name of the item(s) to load
        :return:
        """
        # path to where the possible entries are located
        assert isinstance(items, (list, tuple))

        results = []
        for uuid in items:
            try:
                paths = (
                    op.join(self.root, domain, f)
                    for f in os.listdir(op.join(self.root, domain))
                    if f.startswith(f'{uuid}_')
                )
                _date = None
                result = None
                for path in paths:
                    r = etree.parse(path).getroot()
                    current_date = r.attrib['date']
                    if not _date or _date < current_date:
                        _date = current_date
                        xml = to_string(r)
                        result = {'uuid': r.attrib['uuid'],
                                  'xml': xml}
                if result:
                    results.append(result)
            except ValueError:
                # empty sequence
                continue
        return results

    def list_items(self, domain, item_types=None):
        """
        List items in domain which match item_types if given, or all items
        if not given
        :param domain: domain to list items from
        :param item_types: list or tuple of item_types to list
        :return: a list of dicts where each entry has keys: uuid, item_type
                 and simple_name
        """
        # path to where the possible entries are located
        dir_ = op.join(self.root, domain)
        results = []
        try:
            for fname in os.listdir(dir_):
                path = op.join(self.root, domain, fname)
                if not op.isfile(path):
                    continue
                r = etree.parse(path).getroot()
                if (item_types is None
                        or r.attrib.get('item_type') in item_types):
                    results.append(
                        {'uuid': r.attrib['uuid'],
                         'item_type': r.attrib.get('item_type'),
                         'simple_name': r.attrib.get('simple_name'),
                         'is_trashed': r.attrib.get('is_trashed', 'false'),
                         'user': r.attrib.get('user', ''),
                         'date': r.attrib.get('date', ''),
                         'description': r.attrib.get('description', '')}
                    )
            return results
        except KeyError as e:
            raise ProjectDBError(e)

    def list_named_items(self, domain, item_type, simple_name):
        """
        List items in domain which match item_type and simple_name

        :param domain: domain to list items from
        :param item_type: item_type to match
        :param simple_name: iterable of names to match
        :return: a list of dicts where each entry has keys: uuid, item_type
                 and simple_name
        """
        # path to where the possible entries are located
        dir_ = op.join(self.root, domain)
        results = []
        try:
            for fname in os.listdir(dir_):
                path = op.join(self.root, domain, fname)
                if not op.isfile(path):
                    continue
                r = etree.parse(path).getroot()
                if (r.attrib.get('simple_name') == simple_name and
                        r.attrib.get('item_type') == item_type):
                    results.append(
                        {'uuid': r.attrib['uuid'],
                         'item_type': r.attrib['item_type'],
                         'simple_name': r.attrib['simple_name'],
                         'is_trashed': r.attrib.get('is_trashed', 'false'),
                         'user': r.attrib.get('user', ''),
                         'date': r.attrib.get('date', ''),
                         'description': r.attrib.get('description', '')}
                    )
            return results
        except KeyError as e:
            raise ProjectDBError(e)

    def list_domains(self):
        """
        List top level domains in database
        :return:
        """
        return [dir_
                for dir_ in os.listdir(self.root)
                if op.isdir(op.join(self.root, dir_))]

    def get_configurations_from_device_name(self, domain, instance_id):
        """Returns a list of configurations for a given device

        :param domain: DB domain
        :param instance_id: instance id of the device
        :return: a list of dicts:
            [{"configid": uuid of the configuration,
              "instanceid": device instance uuid in the DB},
              ...
            ]
        """
        dir_ = op.join(self.root, domain)
        results = []
        for fname in os.listdir(dir_):
            path = op.join(self.root, domain, fname)
            if not op.isfile(path):
                continue
            r = etree.parse(path).getroot()
            if (r.attrib.get('item_type') != 'device_instance'):
                continue
            match = f"device_instance[@instance_id='{instance_id}']"
            inst = r.find(match)
            if inst:
                rev = inst.attrib['active_rev']
                for conf in inst.findall(f"device_config[@revision='{rev}']"):
                    results.append({'instanceid': r.attrib['uuid'],
                                    'configid': conf.attrib['uuid']})
        return results

    def get_projects_from_device(self, domain, uuid):
        """
        Returns the projects which contain a device instance with a given uuid

        :param domain: DB domain
        :param uuid: the uuid of the device instance from the database
        :return: a set containing project names
        """

        dir_ = op.join(self.root, domain)
        servers = set()
        for fname in os.listdir(dir_):
            path = op.join(self.root, domain, fname)
            if not op.isfile(path):
                continue
            r = etree.parse(path).getroot()
            if (r.attrib.get('item_type') != 'device_server'):
                continue
            match = f'.//device_instance[@uuid="{uuid}"]'
            if r.findall(match):
                servers.add(r.attrib['uuid'])

        projects = set()

        def add_server(el):
            for server_ in el.findall(".//servers/KRB_Item/uuid"):
                if server_.text in servers:
                    projects.add(el.attrib['simple_name'])
                    return

        for fname in os.listdir(dir_):
            path = op.join(self.root, domain, fname)
            if not op.isfile(path):
                continue
            r = etree.parse(path).getroot()
            if (r.attrib.get('item_type') != 'project'):
                continue
            add_server(r)

        return projects

    def update_attributes(self, items):
        """ Update attribute for the given ``items``

        :param items: list of Hashes containing information on which items
                      to update. Each list entry should be a Hash containing

                      - domain: domain the item resides at
                      - uuid: the uuid of the item
                      - item_type: indicate type of item which attribute should
                                   be changed
                      - attr_name: name of attribute which should be changed
                      - attr_value: value of attribute which should be changed

        :return: a list of dicts where each entry has keys: domain, uuid,
                 item_type, attr_name, attr_value
        """
        res_items = []
        for it in items:
            domain = it.get('domain')
            item_type = it.get('item_type')
            uuid = it.get('uuid')
            attr_name = it.get('attr_name')
            attr_value = it.get('attr_value')
            path = self.path(domain, uuid)
            r = etree.parse(path).getroot()
            if (r.attrib.get('item_type') != item_type):
                continue
            r.attrib[attr_name] = str(attr_value)
            if self.dbhandle.load(to_string(r), path):
                res_items.append(it)
        return res_items
