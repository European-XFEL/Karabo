#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 21, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os
import os.path as op
from sys import platform
from xml.etree.ElementTree import fromstring


class ProjectDBCache(object):
    """ Local storage for project objects retrieved from the project database.
    """
    def __init__(self, dirpath):
        self.dirpath = dirpath

    def flush(self):
        """Flush any pending reads or writes.

        For this class, this is a no-op. In the GUI ProjectDatabaseConnection
        it makes sense to buffer reads and writes.
        """

    def store(self, domain, uuid, revision, data):
        """ Add an object to the cache
        """
        path = self._generate_filepath(domain, uuid, revision)

        domain_dir = op.dirname(path)
        if not op.exists(domain_dir):
            os.mkdir(domain_dir)

        with open(path, mode='w') as fp:
            fp.write(data)

    def retrieve(self, domain, uuid, revision, existing=None):
        """ Read an object from the cache.
        """
        path = self._generate_filepath(domain, uuid, revision)
        if not op.exists(path):
            return None

        with open(path, mode='r') as fp:
            return fp.read()

    def get_uuids_of_type(self, domain, obj_type):
        """ Return ``UUID`` of all objects of ``obj_type`` in cache

        :param domain: A string which describes the domain to search
        :param obj_type: A string which describes the object type
        :return: A list of ``UUID`s for the given ``obj_type``
        """
        domain_dir = op.join(self.dirpath, domain)
        if not op.exists(domain_dir):
            return []

        uuid_list = []
        for fn in os.listdir(domain_dir):
            uuid, revision = self._uuid_revision_from_filename(fn)
            xml = self.retrieve(domain, uuid, revision)
            root = fromstring(xml)
            root_type = root.attrib.get('item_type')
            if root_type == obj_type:
                uuid_list.append(uuid)
        return uuid_list

    def get_available_project_data(self, domain, obj_type):
        """ Return list of Hashes including available project data of all
        objects of ``obj_type`` in cache

        :param domain: A string which describes the domain to search
        :param obj_type: A string which describes the object type
        :return: A list of Hashes for the given ``obj_type`` containing:
                     - 'uuid' - The unique ID of the Project
                     - 'revisions' - A list of revisions for the given project
                     - 'simple_name' - The name for displaying
                     - 'item_type' - Should be project in that case
        """
        domain_dir = op.join(self.dirpath, domain)
        if not op.exists(domain_dir):
            return []

        uuid_revs = {}
        for fn in os.listdir(domain_dir):
            uuid, revision = self._uuid_revision_from_filename(fn)
            xml = self.retrieve(domain, uuid, revision)
            root = fromstring(xml)
            root_type = root.attrib.get('item_type')
            if root_type == obj_type:
                rev_str = root.attrib.get('revision')
                alias = root.attrib.get('alias')
                revHash = {uuid: (int(rev_str), alias),
                           'simple_name': root.attrib.get('simple_name')}
                uuid_revs.setdefault(uuid, []).append(revHash)

        proj_data = []
        for uuid, value in uuid_revs.items():
            simple_name = ''
            revisions = []
            for v in value:
                simple_name = v.get('simple_name', '')
                rev, alias = v.get(uuid, (0, ''))
                revisions.append({'revision': rev,
                                  'alias': alias,
                                  'user': '',
                                  'date': ''})
            proj_data.append({'uuid': uuid,
                              'revisions': revisions,
                              'simple_name': simple_name})
        return proj_data

    def _generate_filepath(self, domain, uuid, revision):
        leafname = '{}_{}'.format(uuid, revision)
        return op.join(self.dirpath, domain, leafname)

    def _uuid_revision_from_filename(self, filename):
        """ Return tuple of ``UUID`` and ``Revision`` from the given
        ``filename``

        :param filename: The file name of an cached object
        :return: A tuple of ``UUID`` and ``Revision`` number
        """
        uuid, revision = filename.split('_')
        return (uuid, revision)


def get_user_cache():
    """ Return a cache residing in the user's home directory.
    """
    if platform.startswith('win'):
        karabo_dir = op.join(os.environ['APPDATA'], 'karabo')
    else:
        karabo_dir = op.join(os.environ['HOME'], '.karabo')
    cache_dir = op.join(karabo_dir, 'project_db_cache')
    os.makedirs(cache_dir, exist_ok=True)
    return ProjectDBCache(cache_dir)
