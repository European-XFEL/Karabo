#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 3, 2016
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################

from qtpy.QtCore import QObject

from karabo.common.api import KARABO_PROJECT_MANAGER, set_modified_flag
from karabo.common.project.api import (
    MemCacheWrapper, get_user_cache, read_lazy_object)
from karabo.common.scenemodel.api import SceneModel
from karabo.native import (
    Hash, get_item_type, read_project_model, write_project_model)
from karabogui import messagebox
from karabogui.events import (
    KaraboEvent, broadcast_event, register_for_broadcasts)
from karabogui.request import get_scene_from_server
from karabogui.singletons.api import get_config, get_network

# This matches the batch size used in the project database
MAX_BUFFER_ITEMS = 50


class ProjectDatabaseConnection(QObject):
    """ An object which handles requests/replies from the GUI server which
    pertain to project data.

    NOTE: Concerning the Cache
    The cache is fundamental to the operation of this database connection. Due
    to the asynchronous nature of reading project data, a cache was
    incorporated to "smooth" things out a bit. In the end, this means that data
    is always delivered to deserialization code from the cache. Structuring
    things this way is a bit complex, but it enables recursively reading
    (with `read_lazy_object`) a project where at any point you might read
    data which is not locally available.

    Just to be clear, the way this works is:
        * An empty object is created with a valid UUID
        * An attempt is made to initialize that object with `read_lazy_object`,
          which is passed a "storage" object which can be a `ProjectDBCache`,
          or an instance of this connection class, or a `MemCacheWrapper` which
          forwards misses to some other storage object.
        * Data which is not immediately available in the storage object is
          requested from the network.
        * Sometime later, data is received asynchronously and written to either
          a memory or disk cache.
        * Objects for which data arrived are read again using
          `read_lazy_object` and this time more of the reads succeed, but yet
          more reads might be generated as well for children of the objects
          which were deserialized.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self.cache = get_user_cache()
        self.network = get_network()

        # Register for broadcast events
        event_map = {
            KaraboEvent.ProjectItemsLoaded: self._event_items_loaded,
            KaraboEvent.ProjectItemsSaved: self._event_items_saved,
        }
        register_for_broadcasts(event_map)

        # Dictionaries to hold items which are awaiting network replies
        self._waiting_for_read = {}
        self._waiting_for_write = {}
        # Lists of Hashes which are buffered before sending to the GUI server
        self._read_items_buffer = []
        self._write_items_buffer = []

        self.project_manager = KARABO_PROJECT_MANAGER
        self._ignore_cache = True

        # XXX: This is really asinine right now!
        self._have_logged_in = False

    # -------------------------------------------------------------------
    # Karabo Events

    def _event_items_loaded(self, data):
        success = data.get('success')
        items = data.get('items', [])
        self._items_loaded(items, success)

    def _event_items_saved(self, data):
        success = data.get('success')
        items = data.get('items', [])
        self._items_saved(items, success)

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

    def get_available_domains(self):
        """ Find out which domains are available
        """
        # Fire and "forget". An event will be broadcast with the reply
        self.network.onListProjectDomains(self.project_manager)
        # Call locally as well
        return self.cache.get_available_domains()

    def get_available_project_data(self, domain, obj_type):
        """ Find out what's available
        """
        if self._ignore_cache:
            # Fire and "forget". An event will be broadcast with the reply
            self.network.onProjectListItems(self.project_manager, domain,
                                            obj_type)
            return []

        # Call locally only if necessary
        return self.cache.get_available_project_data(domain, obj_type)

    def get_projects_with_device(self, domain, device_id):
        """ Find projects which contain configurations for a given device.
        """
        self.network.onProjectListProjectsWithDevice(
            self.project_manager, domain, device_id)

    def get_projects_with_macro(self, domain: str, macro_id: str):
        """ Find projects which contain configurations for a given device.
        """
        self.network.onProjectListProjectsWithMacro(
            self.project_manager, domain, macro_id)

    def get_projects_with_server(self, domain: str, server_id: str):
        """ Find projects which contain configurations for a given server.
        """
        self.network.onProjectListProjectsWithServer(
            self.project_manager, domain, server_id)

    def retrieve(self, domain, uuid, existing=None):
        """Read an object from the database.
        """

        data = None
        if not self._ignore_cache:
            data = self.cache.retrieve(domain, uuid, existing=existing)

        if data is None:
            self._push_reading(domain, uuid, existing)
        return data

    def store(self, domain, uuid, obj):
        """Write an object to the database
        """
        self._push_writing(domain, uuid, obj)

    def update_attribute(self, domain, item_type, uuid, attr_name, attr_value):
        """ Update any attribute of the of the object
        """
        item = Hash('domain', domain, 'item_type', item_type, 'uuid', uuid,
                    'attr_name', attr_name, 'attr_value', attr_value)
        # XXX: TODO send project items
        self.network.onProjectUpdateAttribute(self.project_manager, [item])

    @property
    def default_domain(self):
        return get_config()['domain']

    @default_domain.setter
    def default_domain(self, value):
        get_config()['domain'] = value

    @property
    def ignore_local_cache(self):
        return self._ignore_cache

    @ignore_local_cache.setter
    def ignore_local_cache(self, value):
        self._ignore_cache = value

    def is_reading(self):
        return len(self._waiting_for_read) > 0

    def is_writing(self):
        return len(self._waiting_for_write) > 0

    def is_processing(self):
        return self.is_reading() or self.is_writing()

    def get_database_scene(self, name, uuid, target_window, position=None):
        """Directly ask for a scene from the project database

        This method returns the token for the request.

        Optional keyword arguments:

        - position: The coordinates (List) of the scene
        """
        config = get_config()
        data = {
            "scene_name": name,
            "slot_name": "slotGetScene",
            "target_window": target_window,
            "uuid": uuid,
            "domain": config['domain']}
        if position is not None:
            data["position"] = position

        return get_scene_from_server(self.project_manager, **data)

    # -------------------------------------------------------------------
    # Broadcast event handlers

    def _items_loaded(self, items, success):
        """ A bunch of data just arrived from the network.
        """
        if success:
            # Cache everything locally first only if loading from DB successful
            cache_store = self.cache.store
            for item in items:
                domain = item['domain']
                uuid = item['uuid']
                data = item['xml']
                cache_store(domain, uuid, data)
            storage = self._build_memcache(items)
        else:
            # Project loading failed, use the fast track to tell the world
            self._waiting_for_read.clear()
            self._read_items_buffer = []
            self._broadcast_is_processing(False, bail=True,
                                          loading_failed=True)
            return

        # Then go back through and load any waiting objects
        pop_reading = self._pop_reading
        for item in items:
            domain = item['domain']
            uuid = item['uuid']
            pop_reading(domain, uuid, success, storage)

        # Make a single request to the GUI server
        self.flush()

    def _items_saved(self, items, success):
        """ A bunch of items were just saved
        """
        if success:
            pop_writing = self._pop_writing
            for item in items:
                domain = item['domain']
                uuid = item['uuid']
                entry = item['entry']
                date = entry.get('date', '') if entry is not None else ''
                success = item['success']
                if not success:
                    messagebox.show_error(item['reason'])
                pop_writing(domain, uuid, date, success)
        else:
            # Project writing failed, use the fast track to tell the world
            self._waiting_for_write.clear()
            self._write_items_buffer = []
            self._broadcast_is_processing(False, bail=True)

    # -------------------------------------------------------------------
    # private interface

    def _build_memcache(self, items):
        data = {}
        for item in items:
            domain_data = data.setdefault(item['domain'], {})
            domain_data[item['uuid']] = item['xml']
        return MemCacheWrapper(data, self)

    def _broadcast_is_processing(self, previous_processing, *,
                                 bail=False, loading_failed=False):
        """Create broadcast event and send to all registered ``QObjects``
        """
        if bail:
            # Tell the world reading or writing project failed
            broadcast_event(KaraboEvent.DatabaseIsBusy,
                            {'is_processing': False,
                             'loading_failed': loading_failed})

        # Check current processing state against previous one
        if self.is_processing() == previous_processing:
            return

        # Tell the world
        broadcast_event(KaraboEvent.DatabaseIsBusy,
                        {'is_processing': self.is_processing()})

    def _push_reading(self, domain, uuid, existing):
        # Store previous processing state
        is_processing = self.is_processing()

        if uuid not in self._waiting_for_read:
            assert existing is not None
            self._waiting_for_read[uuid] = existing
            item_type = get_item_type(existing)
            item = Hash('domain', domain, 'uuid', uuid, 'item_type', item_type)
            self._read_items_buffer.append(item)
            if len(self._read_items_buffer) >= MAX_BUFFER_ITEMS:
                self.flush()

        self._broadcast_is_processing(is_processing)

    def _pop_reading(self, domain, uuid, success, storage):
        # Store previous processing state
        is_processing = self.is_processing()

        obj = self._waiting_for_read.pop(uuid, None)
        if obj is not None and success:
            # Only try to read if reading from DB was successful
            read_lazy_object(domain, uuid, storage, read_project_model,
                             existing=obj)

        self._broadcast_is_processing(is_processing)

    def _push_writing(self, domain, uuid, obj):
        # Store previous processing state
        is_processing = self.is_processing()

        # Don't ask the GUI server if you're already waiting for this object
        if uuid not in self._waiting_for_write:
            self._waiting_for_write[uuid] = obj
            # item_type to nofity the clients later
            item_type = get_item_type(obj)
            # Project DB expects xml as string
            xml = write_project_model(obj)
            # XXX overwrite everytime until handled
            item = Hash('domain', domain, 'uuid', uuid,
                        'item_type', item_type, 'xml', xml,
                        'overwrite', True)
            self._write_items_buffer.append(item)
            if len(self._write_items_buffer) >= MAX_BUFFER_ITEMS:
                self.flush()

        self._broadcast_is_processing(is_processing)

    def _pop_writing(self, domain, uuid, date, success):
        # Store previous processing state
        is_processing = self.is_processing()

        obj = self._waiting_for_write.pop(uuid)

        if success:
            # Update date
            obj.date = date
            # Write to the local cache
            data = write_project_model(obj)
            self.cache.store(domain, uuid, data)
            # NOTE: The modified scene model contains one or more widgets
            # that have been modified which need to be reset!
            if isinstance(obj, SceneModel):
                set_modified_flag(obj, False)
            else:
                obj.modified = False

        self._broadcast_is_processing(is_processing)
