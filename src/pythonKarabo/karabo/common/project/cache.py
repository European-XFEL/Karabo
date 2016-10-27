#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 21, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os
import os.path as op
from sys import platform


class ProjectDBCache(object):
    """ Local storage for project objects retrieved from the project database.
    """
    def __init__(self, dirpath):
        self.dirpath = dirpath

    def store(self, uuid, revision, data):
        """ Add an object to the cache
        """
        with open(self._generate_filepath(uuid, revision), mode='wb') as fp:
            fp.write(data)

    def retrieve(self, uuid, revision):
        """ Read an object from the cache.
        """
        path = self._generate_filepath(uuid, revision)
        if not op.exists(path):
            msg = 'Cache object for UUID: {} Revision: {} not found'
            raise FileNotFoundError(msg.format(uuid, revision))

        with open(path, mode='rb') as fp:
            return fp.read()

    def _generate_filepath(self, uuid, revision):
        leafname = '{}_{}'.format(uuid, revision)
        return op.join(self.dirpath, leafname)


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
