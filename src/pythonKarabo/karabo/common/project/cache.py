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

    def store(self, domain, uuid, revision, data):
        """ Add an object to the cache
        """
        path = self._generate_filepath(domain, uuid, revision)

        domain_dir = op.dirname(path)
        if not op.exists(domain_dir):
            os.mkdir(domain_dir)

        with open(path, mode='wb') as fp:
            fp.write(data)

    def retrieve(self, domain, uuid, revision):
        """ Read an object from the cache.
        """
        path = self._generate_filepath(domain, uuid, revision)
        if not op.exists(path):
            msg = 'Cache object for UUID: {} Revision: {} not found'
            raise FileNotFoundError(msg.format(uuid, revision))

        with open(path, mode='rb') as fp:
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
