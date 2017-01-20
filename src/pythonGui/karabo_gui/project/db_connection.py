#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 3, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QObject

from karabo.common.project.api import get_user_cache, read_lazy_object
from karabo.common.savable import set_modified_flag
from karabo.middlelayer import Hash
from karabo.middlelayer_api.project.api import read_project_model
from karabo_gui.events import (
    register_for_broadcasts, KaraboBroadcastEvent, KaraboEventSender)
from karabo_gui.singletons.api import get_network


class ProjectDatabaseConnection(QObject):
    """ An object which handles requests/replies from the GUI server which
    pertain to project data.
    """

    def __init__(self, parent=None):
        super(ProjectDatabaseConnection, self).__init__(parent)
        self.cache = get_user_cache()
        self.network = get_network()

        # Register for broadcast events
        register_for_broadcasts(self)

        # Dictionaries to hold items which are awaiting network replies
        self._waiting_for_read = {}
        self._waiting_for_write = {}

        # XXX: Temporary
        self.project_manager = 'KaraboProjectDB'

        # XXX: This is really asinine right now!
        self._have_logged_in = False

    def eventFilter(self, obj, event):
        """ Router for incoming broadcasts
        """
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.ProjectItemsLoaded:
                items = event.data.get('items', [])
                self._items_loaded(items)
            elif event.sender is KaraboEventSender.ProjectItemsSaved:
                items = event.data.get('items', [])
                self._items_saved(items)
            return False
        return super(ProjectDatabaseConnection, self).eventFilter(obj, event)

    # -------------------------------------------------------------------
    # User interface

    def get_uuids_of_type(self, domain, obj_type):
        """ Find out what's available
        """
        # XXX: Please don't keep this here!
        self._ensure_login()

        # Fire and "forget". An event will be broadcast with the reply
        self.network.onProjectListItems(self.project_manager, domain, obj_type)
        # Call locally as well
        cached = self.cache.get_uuids_of_type(domain, obj_type)
        return cached

    def get_available_project_data(self, domain, obj_type):
        """ Find out what's available
        """
        # XXX: Please don't keep this here!
        self._ensure_login()

        # Fire and "forget". An event will be broadcast with the reply
        self.network.onProjectListItems(self.project_manager, domain, obj_type)
        # Call locally as well
        return self.cache.get_available_project_data(domain, obj_type)

    def retrieve(self, domain, uuid, revision, existing=None):
        """Read an object from the database.
        """
        # XXX: Please don't keep this here!
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

    def store(self, domain, uuid, revision, data, obj=None):
        """Write an object to the database
        """
        # XXX: Please don't keep this here!
        self._ensure_login()

        key = (uuid, revision)

        # Don't ask the GUI server if you're already waiting for this object
        if key not in self._waiting_for_write:
            self._waiting_for_write[key] = (obj, data)
            items = [Hash('domain', domain, 'uuid', uuid, 'revision', revision,
                          'xml', data, 'overwrite', False)]
            self.network.onProjectSaveItems(self.project_manager, items)

    # -------------------------------------------------------------------
    # Broadcast event handlers

    def _items_loaded(self, items):
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
                # Loading will cause the modified flag to flip!
                set_modified_flag(obj, value=False)

    def _items_saved(self, items):
        """ A bunch of items were just saved
        """
        for item in items:
            domain = item['domain']
            uuid = item['uuid']
            revision = item['revision']
            if item['success']:
                key = (uuid, revision)
                obj, data = self._waiting_for_write.pop(key)

                # Write to the local cache
                self.cache.store(domain, uuid, revision, data, obj)
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
