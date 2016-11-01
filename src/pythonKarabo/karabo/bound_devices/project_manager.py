from karabo.bound import (AccessLevel, BOOL_ELEMENT, Hash, KARABO_CLASSINFO,
                          OVERWRITE_ELEMENT, PythonDevice,
                          SLOT_ELEMENT, STRING_ELEMENT, UINT32_ELEMENT)
from karabo.common.states import State
from karabo.project_db.project_database import ProjectDatabase
from karabo.project_db.util import assure_running, ProjectDBError


def dictToHash(d):
    h = Hash()
    for k, v in d.items():
        if isinstance(v, dict):
            h.set(k, dictToHash(v))
        elif isinstance(v, (list, tuple)):
            if len(v) > 0 and isinstance(v[0], dict):
                h.set(k, [dictToHash(vv) for vv in v])
            else:
                h.set(k, v)
        else:
            h.set(k, v)
    return h


@KARABO_CLASSINFO("ProjectManager", "2.0")
class ProjectManager(PythonDevice):

    @staticmethod
    def expectedParameters(expected):

        (
            OVERWRITE_ELEMENT(expected).key('deviceId')
                .setNewDefaultValue("ProjectService")
                .commit()
            ,
            OVERWRITE_ELEMENT(expected).key('visibility')
                .setNewDefaultValue(AccessLevel.ADMIN)
                .commit()
            ,
            STRING_ELEMENT(expected).key('host')
                .displayedName("Database host")
                .expertAccess()
                .assignmentOptional().defaultValue('localhost')
                .reconfigurable()
                .commit()
            ,
            STRING_ELEMENT(expected).key('domain')
                .displayedName("Karabo domain")
                .expertAccess()
                .assignmentOptional().defaultValue('LOCAL')
                .reconfigurable()
                .commit()
            ,
            UINT32_ELEMENT(expected).key('port')
                .displayedName("Database port")
                .expertAccess()
                .assignmentOptional().defaultValue(8080)
                .reconfigurable()
                .commit()
            ,
            SLOT_ELEMENT(expected).key('reset')
                .displayedName("Reset")
                .allowedStates(State.ERROR)
                .expertAccess()
                .commit()
            ,
            BOOL_ELEMENT(expected).key('testMode')
                .displayedName("Test Mode")
                .assignmentOptional().defaultValue(False)
                .adminAccess()
                .commit()
            ,
        )

    def __init__(self, config):
        super(ProjectManager, self).__init__(config)
        self.registerSlot(self.reset)
        self.registerSlot(self.slotInitUserSession)
        self.registerSlot(self.slotEndUserSession)
        self.registerSlot(self.slotSaveItems)
        self.registerSlot(self.slotLoadItems)
        self.registerSlot(self.slotLoadItemsAndSubs)
        self.registerSlot(self.slotGetVersionInfo)
        self.registerSlot(self.slotListItems)
        self.registerInitialFunction(self.initialization)
        self.db = None

    def initialization(self):
        """
        Initialization function of this device. Checks that the configured
        database is indeed reachable and accessible

        If the database is reachable updates the device state to NORMAL,
        otherwise brings the device into ERROR
        """
        # check if we can connect to the database
        try:
            assure_running(self.get("host"), self.get("port"))
            self.updateState(State.NORMAL)
        except ProjectDBError:
            self.updateState(State.ERROR)

    def reset(self):
        """
        Resetting the device brings it into `State.INIT`
        :return:
        """
        self.updateState(State.INIT)
        self.initialization()

    def _getCurrentConfig(self):
        """
        Return database relevant configuration from expected parameters
        as tuple
        :return: at tuple of host, port and domain
        """
        return self.get("host"), self.get("port"), self.get("domain")

    def slotInitUserSession(self, user, password):
        """
        Initialize a DB connection for a user
        :param user:
        :param password:
        """
        host, port, domain = self._getCurrentConfig()
        self.db = ProjectDatabase(user, password,
                                  server=host,
                                  port=port,
                                  test_mode=self.get("testMode"))

        self.log.DEBUG("Initialized user session")

    def slotEndUserSession(self):
        """
        End a user session
        """
        self.db = None

        self.log.DEBUG("Ended user session")

    def _checkDbInitialized(self):
        if self.db is None:
            raise RuntimeError("You need to init a database session first")

    def slotSaveItems(self, items):
        """
        Save items in project database
        :param items: items to be save. Should be a list(Hash) object were each
                      entry is of the form:

                      - xml: xml of item
                      - uuid: uuid of item
                      - revision: revision of item
                      - overwrite: behavior in case of revision conflict
                      - domain: to write to

        :raises: `ProjectDBError` in case of database (connection) problems
                 `TypeError` in case an no type information or an unknown type
                 is found in the root element.
                 `RuntimeError` if no database if connected.
        """

        self.log.DEBUG("Saving items: {}".format([i.get("uuid") for i in
                                                  items]))

        self._checkDbInitialized()

        results = Hash()
        with self.db:
            for item in items:
                xml = item.get("xml")
                uuid = item.get("uuid")
                overwrite = item.get("overwrite")
                domain = item.get("domain")
                success, meta = self.db.save_item(domain, uuid, xml, overwrite)
                results.set(uuid, Hash('success', success,
                                       'entry', dictToHash(meta)))

        self.reply(results)

    def slotLoadItems(self, domain, items):
        """
        Loads items from the database
        :param domain: domain to load items from
        :param items: list of Hashes containing information on which items
                      to load. Each list entry should be a Hash containing

                      - uuid: the uuid of the item
                      - revision (optional): the revision to load. Leave empty
                          if the newest is to be loaded

        :return: a Hash where the keys are the item uuids and values are the
                 item XML. If the load failed the value for this uuid is set
                 to False
        """

        self.log.DEBUG("Loading items for domain '{}': {}".format(domain,
                       [i.get("uuid") for i in items]))

        self._checkDbInitialized()

        loadedItems = Hash()
        with self.db:
            for item in items:
                uuid = item.get("uuid")
                revision = None
                if item.has("revision"):
                    revision = item.get("revision")
                it = self.db.load_item(domain, uuid, revision)
                loadedItems.set(uuid, it)

        self.reply(loadedItems)

    def slotLoadItemsAndSubs(self, domain, items, list_tags):
        """
        Loads items from the database - including any sub items. For this
        the root element of each item is expected to have an attribute
        list_tag, identifying under which tag to find child items. These
        will then be loaded.

        :param domain: domain to load items from
        :param items: list of Hashes containing information on which items
                      to load. Each list entry should be a Hash containing

                      - uuid: the uuid of the item
                      - revision (optional): the revision to load. Leave empty
                          if the newest is to be loaded

        :return: a Hash where the keys are the item uuids and values are the
                 item XML. If the load failed the value for this uuid is set
                 to False
        """

        self.log.DEBUG("Loading multiple for domain '{}': {}".format(domain,
                       [i.get("uuid") for i in items]))

        self._checkDbInitialized()

        loadedItems = Hash()
        with self.db:
            for item in items:
                uuid = item.get("uuid")
                revision = None
                if item.has("revision"):
                    revision = item.get("revision")
                itxml = self.db.load_item(domain, uuid, revision)
                loadedItems.set(uuid, itxml)

                if itxml != "":  # load succeeded check for children
                    its = self.db.load_multi(domain, itxml, list_tags)
                    [loadedItems.set(k, v) for k, v in its.items()]

        self.reply(loadedItems)

    def slotGetVersionInfo(self, domain, items):
        """
        Retrieve versioning information from the database for a list of items
        :param domain: the item is to be found at
        :param items: list of Hashes containing information on which items
                      to load. Each list entry should be a Hash containing

                      - uuid: the uuid of the item
        :return: A Hash where the keys are the uuid of the item and the values
                 are Hashes holding the versioning information for this item.
                 These Hashes are of the form:

                - document: the path of the document
                - revisions: a list of revisions, where each entry is a dict
                            with:
                            * id: the revision id
                            * date: the date this revision was added
                            * user: the user that added this revision
        """

        self.log.DEBUG("Retrieving version info for domain '{}': {}"
                       .format(domain, [i.get("uuid") for i in items]))

        self._checkDbInitialized()

        versionInfos = Hash()
        with self.db:
            for item in items:
                uuid = item.get("uuid")
                vers = self.db.get_versioning_info_item(domain, uuid)
                # now convert the dict into a Hash
                versionInfos.set(uuid, dictToHash(vers))

        self.reply(versionInfos)

    def slotListItems(self, domain, item_types=None):
        """
        List items in domain which match item_types if given, or all items
        if not given
        :param domain: domain to list items from
        :param item_types: list or tuple of item_types to list
        :return: a list of Hashes where each entry has keys: uuid, item_type
                 and simple_name
        """
        resHashed = []
        with self.db:
            res = self.db.list_items(domain, item_types)
            for r in res:
                h = Hash('uuid', r['uuid'],
                         'item_type', r['item_type'],
                         'simple_name', r['simple_name'])
                resHashed.append(h)
        self.reply(resHashed)
