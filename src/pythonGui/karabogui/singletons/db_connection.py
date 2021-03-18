#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 3, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt5.QtCore import QObject

from karabo.common.api import set_modified_flag, KARABO_PROJECT_MANAGER
from karabo.common.project.api import (
    MemCacheWrapper, get_user_cache, read_lazy_object)
from karabo.common.scenemodel.api import SceneModel
from karabo.native import (
    get_item_type, Hash, read_project_model, write_project_model)
from karabogui import messagebox
from karabogui.events import (
    broadcast_event, KaraboEvent, register_for_broadcasts
)
from karabogui.singletons.api import get_config, get_network
from karabogui.request import call_device_slot, handle_scene_from_server

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
        super(ProjectDatabaseConnection, self).__init__(parent)
        self.cache = get_user_cache()
        self.network = get_network()

        # Register for broadcast events
        event_map = {
            KaraboEvent.ProjectItemsLoaded: self._event_items_loaded,
            KaraboEvent.ProjectItemsSaved: self._event_items_saved,
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.ProjectDBConnect: self._event_db_connect
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

    def _event_network(self, data):
        self._have_logged_in = False

    def _event_db_connect(self, data):
        # NOTE: The ProjectDB came online after we have logged into the
        # GUI server. Hence we have to reestablish our connection to
        # the project database.
        instance_id = data.get('device', '')
        if instance_id == self.project_manager:
            self._have_logged_in = False
            self._ensure_login()

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
        # XXX: Please don't keep this here!
        self._ensure_login()

        # Fire and "forget". An event will be broadcast with the reply
        self.network.onListProjectDomains(self.project_manager)
        # Call locally as well
        return self.cache.get_available_domains()

    def get_available_project_data(self, domain, obj_type):
        """ Find out what's available
        """
        # XXX: Please don't keep this here!
        self._ensure_login()

        if self._ignore_cache:
            # Fire and "forget". An event will be broadcast with the reply
            self.network.onProjectListItems(self.project_manager, domain,
                                            obj_type)
            return []

        # Call locally only if necessary
        return self.cache.get_available_project_data(domain, obj_type)

    def retrieve(self, domain, uuid, existing=None):
        """Read an object from the database.
        """
        # XXX: Please don't keep this here!
        self._ensure_login()

        data = None
        if not self._ignore_cache:
            data = self.cache.retrieve(domain, uuid, existing=existing)

        if data is None:
            self._push_reading(domain, uuid, existing)
        return data

    def store(self, domain, uuid, obj):
        """Write an object to the database
        """
        # XXX: Please don't keep this here!
        self._ensure_login()
        self._push_writing(domain, uuid, obj)

    def update_attribute(self, domain, item_type, uuid, attr_name, attr_value):
        """ Update any attribute of the of the object
        """
        # XXX: Please don't keep this here!
        self._ensure_login()

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

    # -------------------------------------------------------------------
    # Broadcast event handlers

    def _items_loaded(self, items, success):
        """ A bunch of data just arrived from the network.
        """
        # To be passed to _pop_reading
        storage = self
        if success:
            # Cache everything locally first only if loading from DB successful
            for item in items:
                domain = item['domain']
                uuid = item['uuid']
                data = item['xml']
                self.cache.store(domain, uuid, data)
            storage = self._build_memcache(items)
        else:
            # Project loading failed, use the fast track to tell the world
            self._waiting_for_read.clear()
            self._read_items_buffer = []
            self._broadcast_is_processing(False, bail=True,
                                          loading_failed=True)
            return

        # Then go back through and load any waiting objects
        for item in items:
            domain = item['domain']
            uuid = item['uuid']
            self._pop_reading(domain, uuid, success, storage)

        # Make a single request to the GUI server
        self.flush()

    def _items_saved(self, items, success):
        """ A bunch of items were just saved
        """
        if success:
            for item in items:
                domain = item['domain']
                uuid = item['uuid']
                entry = item['entry']
                date = entry.get('date', '') if entry is not None else ''
                success = item['success']
                if not success:
                    messagebox.show_error(item['reason'])
                self._pop_writing(domain, uuid, date, success)
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

    def _ensure_login(self):
        if not self._have_logged_in:
            self.network.onProjectBeginSession(self.project_manager)
            self._have_logged_in = True

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
            item = Hash('domain', domain, 'uuid', uuid)
            self._read_items_buffer.append(item)
            if len(self._read_items_buffer) >= MAX_BUFFER_ITEMS:
                self.flush()

        self._broadcast_is_processing(is_processing)

    def _pop_reading(self, domain, uuid, success, storage):
        # Store previous processing state
        is_processing = self.is_processing()

        if uuid in self._waiting_for_read:
            obj = self._waiting_for_read.pop(uuid)
            if success:
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

    def _get_database_scene(self, name, uuid, target_window):
        """Directly ask for a scene from the project database
        """
        config = get_config()
        device_id = self.project_manager
        domain = config['domain']
        db_token = config['db_token']
        handler = partial(handle_scene_from_server, device_id, name, None,
                          target_window)
        call_device_slot(handler, device_id, 'slotGetScene',
                         uuid=uuid, domain=domain, db_token=db_token)
