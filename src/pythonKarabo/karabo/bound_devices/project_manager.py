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
            OVERWRITE_ELEMENT(expected).key('_deviceId_')
            .setNewDefaultValue("ProjectService")
            .commit(),
            OVERWRITE_ELEMENT(expected).key('visibility')
            .setNewDefaultValue(AccessLevel.ADMIN)
            .commit(),
            STRING_ELEMENT(expected).key('host')
            .displayedName("Database host")
            .expertAccess()
            .assignmentOptional().defaultValue('localhost')
            .reconfigurable()
            .commit(),
            UINT32_ELEMENT(expected).key('port')
            .displayedName("Database port")
            .expertAccess()
            .assignmentOptional().defaultValue(8080)
            .reconfigurable()
            .commit(),
            SLOT_ELEMENT(expected).key('reset')
            .displayedName("Reset")
            .allowedStates(State.ERROR)
            .expertAccess()
            .commit(),
            BOOL_ELEMENT(expected).key('testMode')
            .displayedName("Test Mode")
            .assignmentOptional().defaultValue(False)
            .adminAccess()
            .commit(),
        )

    def __init__(self, config):
        super(ProjectManager, self).__init__(config)
        self.registerSlot(self.reset)
        self.registerSlot(self.slotBeginUserSession)
        self.registerSlot(self.slotEndUserSession)
        self.registerSlot(self.slotSaveItems)
        self.registerSlot(self.slotLoadItems)
        self.registerSlot(self.slotLoadItemsAndSubs)
        self.registerSlot(self.slotGetVersionInfo)
        self.registerSlot(self.slotListItems)
        self.registerInitialFunction(self.initialization)
        self.user_db_sessions = {}

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
        :return: at tuple of host, port
        """
        return self.get("host"), self.get("port")

    def slotBeginUserSession(self, token):
        """
        Initialize a DB connection for a user
        :param token: database user token
        """
        # XXX: Leave these hardcoded until session tokens are working
        user, password = "admin", "karabo"
        host, port = self._getCurrentConfig()
        if host != "localhost":
            user = "karabo"
        db = ProjectDatabase(user, password,
                             server=host,
                             port=port,
                             test_mode=self.get("testMode"))
        self.user_db_sessions[token] = db

        self.log.DEBUG("Initialized user session")
        self.reply(Hash("success", True))

    def slotEndUserSession(self, token):
        """
        End a user session
        :param token: database user token
        """
        del self.user_db_sessions[token]

        self.log.DEBUG("Ended user session")
        self.reply(Hash("success", True))

    def _checkDbInitialized(self, token):
        if token not in self.user_db_sessions:
            raise RuntimeError("You need to init a database session first")

    def slotSaveItems(self, token, items):
        """
        Save items in project database
        :param token: database user token
        :param items: items to be save. Should be a list(Hash) object were each
                      entry is of the form:

                      - xml: xml of item
                      - uuid: uuid of item
                      - overwrite: behavior in case of revision conflict
                      - domain: to write to

        :raises: `ProjectDBError` in case of database (connection) problems
                 `TypeError` in case an no type information or an unknown type
                 is found in the root element.
                 `RuntimeError` if no database if connected.
        """

        self.log.DEBUG("Saving items: {}".format([i.get("uuid") for i in
                                                  items]))
        self._checkDbInitialized(token)

        savedItems = []
        with self.user_db_sessions[token] as db_session:
            for item in items:
                xml = item.get("xml")
                uuid = item.get("uuid")
                overwrite = item.get("overwrite")
                domain = item.get("domain")
                exceptionReason = ""
                success = True
                meta = None
                try:
                    meta = db_session.save_item(domain, uuid, xml, overwrite)
                    meta = dictToHash(meta)
                except ProjectDBError as e:
                    success = False
                    exceptionReason = str(e)
                item.set("success", success)
                item.set("reason", exceptionReason)
                item.set("entry", meta)
                savedItems.append(item)
        self.reply(Hash('items', savedItems))

    def slotLoadItems(self, token, items):
        """
        Loads items from the database

        :param token: database user token
        :param items: list of Hashes containing information on which items
                      to load. Each list entry should be a Hash containing

                      - uuid: the uuid of the item
                      - revision (optional): the revision to load. Leave empty
                          if the newest is to be loaded
                      - domain: domain to load item from

        :return: a Hash where the keys are the item uuids and values are the
                 item XML. If the load failed the value for this uuid is set
                 to False
        """

        self.log.DEBUG("Loading items: {}"
                       .format([i.get("uuid") for i in items]))

        self._checkDbInitialized(token)

        exceptionReason = ""
        success = True
        loadedItems = []
        with self.user_db_sessions[token] as db_session:
            # verify that items belong to single domain
            domain = items[0].get("domain")
            keys = [(it.get('uuid'), it.get('revision')) for it in items
                    if it.get('domain') == domain]
            assert len(keys) == len(items), "Incorrect domain given!"
            uuids, revs = zip(*keys)

            try:
                items = db_session.load_item(domain, uuids, revs)
                for item in items:
                    h = Hash("domain", domain,
                             "uuid", item["uuid"],
                             "revision", item["revision"],
                             "xml", item["xml"])
                    loadedItems.append(h)

            except ProjectDBError as e:
                exceptionReason = str(e)
                success = False
        self.reply(Hash('items', loadedItems,
                        'success', success,
                        'reason', exceptionReason))

    def slotLoadItemsAndSubs(self, token, items):
        """
        Loads items from the database - including any sub items.

        :param token: database user token
        :param items: list of Hashes containing information on which items
                      to load. Each list entry should be a Hash containing

                      - uuid: the uuid of the item
                      - revision (optional): the revision to load. Leave empty
                          if the newest is to be loaded
                      - domain: domain to load item from

        :return: a Hash where the keys are the item uuids and values are the
                 item XML. If the load failed the value for this uuid is set
                 to False
        """

        self.log.DEBUG("Loading multiple: {}"
                       .format([i.get("uuid") for i in items]))

        self._checkDbInitialized(token)

        loadedItems = []
        self.reply(Hash('items', loadedItems))

    def slotGetVersionInfo(self, token, items):
        """
        Retrieve versioning information from the database for a list of items

        :param token: database user token
        :param domain: the item is to be found at
        :param items: list of Hashes containing information on which items
                      to load. Each list entry should be a Hash containing

                      - uuid: the uuid of the item
                      - domain: domain to load item from

        :return: A Hash where the keys are the uuid of the item and the values
                 are Hashes holding the versioning information for this item.
                 These Hashes are of the form:

                - document: the path of the document
                - revisions: a list of revisions, where each entry is a dict
                            with:
                            * revision: the revision number
                            * date: the date this revision was added
                            * user: the user that added this revision
                            * alias: an alias for the revision. Will return the
                                     revision number if none is set.
        """

        self.log.DEBUG("Retrieving version info: {}"
                       .format([i.get("uuid") for i in items]))

        self._checkDbInitialized(token)

        versionInfos = Hash()
        with self.user_db_sessions[token] as db_session:
            for item in items:
                domain = item.get("domain")
                uuid = item.get("uuid")
                vers = db_session.get_versioning_info(domain, uuid)
                # now convert the dict into a Hash
                versionInfos.set(uuid, dictToHash(vers))

        self.reply(versionInfos)

    def slotListItems(self, token, domain, item_types=None):
        """
        List items in domain which match item_types if given, or all items
        if not given
        :param token: database user token
        :param domain: domain to list items from
        :param item_types: list or tuple of item_types to list
        :return: a list of Hashes where each entry has keys: uuid, item_type
                 and simple_name
        """

        self._checkDbInitialized(token)

        with self.user_db_sessions[token] as db_session:
            exceptionReason = ""
            success = True
            resHashes = []
            try:
                res = db_session.list_items(domain, item_types)
                for r in res:
                    revisions = [dictToHash(rev) for rev in r['revisions']]
                    h = Hash('uuid', r['uuid'],
                             'revisions', revisions,
                             'item_type', r['item_type'],
                             'simple_name', r['simple_name'])
                    resHashes.append(h)
            except ProjectDBError as e:
                exceptionReason = str(e)
                success = False
            self.reply(Hash('items', resHashes,
                            'success', success,
                            'reason', exceptionReason))

    def slotListDomains(self, token):
        """
        List domains available on this database
        :param token: database user token
        :return:
        """

        self._checkDbInitialized(token)

        with self.user_db_sessions[token] as db_session:
            res = db_session.list_domains()
            self.reply(Hash('domains', res))
