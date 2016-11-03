#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 3, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from karabo.common.project.api import get_user_cache, read_lazy_object
from karabo.middlelayer import Hash
from karabo.middlelayer_api.newproject.io import (read_project_model,
                                                  write_project_model)
from karabo_gui.network import Network


class ProjectDatabaseConnection(object):
    """ An object which handles requests/replies from the GUI server which
    pertain to project data.
    """

    def __init__(self):
        self.cache = get_user_cache()
        self.network = Network()

        # XXX: Temporary
        self.project_manager = 'Karabo_ProjectServerDB_ProjectManager_1'

        # Dictionaries to hold items which are awaiting network replies
        self._waiting_for_read = {}
        self._waiting_for_write = {}

        # XXX: This is really fucking asinine right now!
        self._have_logged_in = False

    # -------------------------------------------------------------------
    # User interface

    def get_uuids_of_type(self, domain, obj_type):
        """ Find out what's available
        """
        self._ensure_login()

        # Fire and "forget". An event will be broadcast with the reply
        self.network.onProjectListItems(self.project_manager, domain, obj_type)
        # Call locally as well
        cached = self.cache.get_uuids_of_type(domain, obj_type)
        return cached

    def retrieve(self, domain, uuid, revision, existing=None):
        """Read an object from the database.
        """
        self._ensure_login()

        obj = self.cache.retrieve(domain, uuid, revision, existing=existing)
        if obj is None:
            key = (uuid, revision)
            if key not in self._waiting_for_read:
                items = [Hash('domain', domain, 'uuid', uuid,
                              'revision', revision)]
                self.network.onProjectLoadItems(self.project_manager, items)
                if existing is not None:
                    self._waiting_for_read[key] = existing
        return obj

    def store(self, domain, uuid, revision, obj):
        """Write an object to the database
        """
        self._ensure_login()

        key = (uuid, revision)

        # Don't ask the GUI server if you're already waiting for this object
        if key not in self._waiting_for_write:
            self._waiting_for_write[key] = obj
            xml = write_project_model(obj)
            items = [Hash('domain', domain, 'uuid', uuid, 'revision', revision,
                          'xml', xml, 'overwrite', False)]
            self.network.onProjectSaveItems(self.project_manager, items)

    # -------------------------------------------------------------------
    # Manager interface

    def items_loaded(self, items):
        """ A bunch of data just arrived from the network.
        """
        # Cache everything locally first
        for item in items:
            domain = item['domain']
            uuid = item['uuid']
            revision = item['revision']
            data = item['xml']
            self.cache.store(domain, uuid, revision, data)

        # Then go back through and load any waiting objects
        for item in items:
            domain = item['domain']
            uuid = item['uuid']
            revision = item['revision']
            key = (uuid, revision)
            if key in self._waiting_for_read:
                obj = self._waiting_for_read.pop(key)
                read_lazy_object(domain, uuid, revision, self,
                                 read_project_model, existing=obj)

    def items_saved(self, items):
        """ A bunch of items were just saved
        """
        for item in items:
            entry = item['entry']
            uuid = entry['uuid']
            domain = entry['domain']
            revision = entry['revision']
            if item['success']:
                vers_info = entry['versioning_info']
                revisions = vers_info['revisions']
                new_revision = revisions[-1]['revision']

                key = (uuid, revision)
                obj = self._waiting_for_write.pop(key)

                obj.revision = new_revision
                data = write_project_model(obj)
                self.cache.store(domain, uuid, new_revision, data)
                # No longer dirty!
                obj.modified = False
            else:
                # XXX: Make some noise.
                # Right now, only the modified flag doesn't get updated.
                key = (uuid, revision)
                self._waiting_for_write.pop(key)

    # -------------------------------------------------------------------
    # private interface

    def _ensure_login(self):
        if not self._have_logged_in:
            self.network.onProjectBeginSession(self.project_manager)
            self._have_logged_in = True
