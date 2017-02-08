#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 3, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QObject

from karabo.common.project.api import get_user_cache, read_lazy_object
from karabo.middlelayer import Hash
from karabo.middlelayer_api.project.api import (read_project_model,
                                                write_project_model)
from karabo_gui.events import (
    broadcast_event, KaraboBroadcastEvent, KaraboEventSender,
    register_for_broadcasts
)
from karabo_gui.messagebox import MessageBox
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
        # Lists of Hashes which are buffered before sending to the GUI server
        self._read_items_buffer = []
        self._write_items_buffer = []

        # XXX: Temporary
        self.project_manager = 'KaraboProjectDB'

        # XXX: This is really asinine right now!
        self._have_logged_in = False

    def eventFilter(self, obj, event):
        """ Router for incoming broadcasts
        """
        if isinstance(event, KaraboBroadcastEvent):
            data = event.data
            if event.sender is KaraboEventSender.ProjectItemsLoaded:
                success = data.get('success')
                items = data.get('items', [])
                self._items_loaded(items, success)
            elif event.sender is KaraboEventSender.ProjectItemsSaved:
                items = data.get('items', [])
                self._items_saved(items)
            return False
        return super(ProjectDatabaseConnection, self).eventFilter(obj, event)

    # -------------------------------------------------------------------
    # User interface

    def flush(self):
        """Flush any pending reads or writes.
        """
        if len(self._read_items_buffer) > 0:
            items = self._read_items_buffer
            self.network.onProjectLoadItems(self.project_manager, items)
            self._read_items_buffer = []
        if len(self._write_items_buffer) > 0:
            items = self._write_items_buffer
            self.network.onProjectSaveItems(self.project_manager, items)
            self._write_items_buffer = []

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
            self._push_reading(domain, uuid, revision, existing)
        return obj

    def store(self, domain, uuid, revision, obj):
        """Write an object to the database
        """
        # XXX: Please don't keep this here!
        self._ensure_login()

        self._push_writing(domain, uuid, revision, obj)

    def is_reading(self):
        return len(self._waiting_for_read) > 0

    def is_writing(self):
        return len(self._waiting_for_write) > 0

    def is_processing(self):
        return self.is_reading() or self.is_writing()

    # -------------------------------------------------------------------
    # Broadcast event handlers

    def _items_loaded(self, items, success):
        """ A bunch of data just arrived from the network.
        """
        if success:
            # Cache everything locally first only if loading from DB successful
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
            self._pop_reading(domain, uuid, revision, success)

        # Make a single request to the GUI server
        self.flush()

    def _items_saved(self, items):
        """ A bunch of items were just saved
        """
        for item in items:
            domain = item['domain']
            uuid = item['uuid']
            revision = item['revision']
            success = item['success']
            if not success:
                MessageBox.showError(item['reason'])
            self._pop_writing(domain, uuid, revision, success)

    # -------------------------------------------------------------------
    # private interface

    def _ensure_login(self):
        if not self._have_logged_in:
            self.network.onProjectBeginSession(self.project_manager)
            self._have_logged_in = True

    def _broadcast_is_processing(self, previous_processing):
        """Create broadcast event and send to all registered ``QObjects``
        """
        # Check current processing state against previous one
        if self.is_processing() == previous_processing:
            return

        data = {'is_processing': self.is_processing()}
        # Create KaraboBroadcastEvent
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.DatabaseIsBusy, data))

    def _push_reading(self, domain, uuid, revision, existing):
        # Store previous processing state
        is_processing = self.is_processing()

        key = (uuid, revision)
        if key not in self._waiting_for_read:
            assert existing is not None
            self._waiting_for_read[key] = existing
            item = Hash('domain', domain, 'uuid', uuid, 'revision', revision)
            self._read_items_buffer.append(item)

        self._broadcast_is_processing(is_processing)

    def _pop_reading(self, domain, uuid, revision, success):
        # Store previous processing state
        is_processing = self.is_processing()

        key = (uuid, revision)
        if key in self._waiting_for_read:
            obj = self._waiting_for_read.pop(key)
            if success:
                # Only try to read if reading from DB was successful
                read_lazy_object(domain, uuid, revision, self,
                                 read_project_model, existing=obj)

        self._broadcast_is_processing(is_processing)

    def _push_writing(self, domain, uuid, revision, obj):
        # Store previous processing state
        is_processing = self.is_processing()

        key = (uuid, revision)
        # Don't ask the GUI server if you're already waiting for this object
        if key not in self._waiting_for_write:
            self._waiting_for_write[key] = obj
            # Project DB expects xml as string
            xml = write_project_model(obj)
            # XXX overwrite everytime until handled
            item = Hash('domain', domain, 'uuid', uuid, 'revision', revision,
                        'xml', xml, 'overwrite', True)
            self._write_items_buffer.append(item)

        self._broadcast_is_processing(is_processing)

    def _pop_writing(self, domain, uuid, revision, success):
        # Store previous processing state
        is_processing = self.is_processing()

        key = (uuid, revision)
        obj = self._waiting_for_write.pop(key)

        if success:
            # Write to the local cache
            data = write_project_model(obj)
            self.cache.store(domain, uuid, revision, data)
            # No longer dirty!
            obj.modified = False

        self._broadcast_is_processing(is_processing)
