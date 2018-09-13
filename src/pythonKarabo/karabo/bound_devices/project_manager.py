from io import StringIO
from lxml import etree

from karabo.bound import (AccessLevel, BOOL_ELEMENT, Hash, KARABO_CLASSINFO,
                          OVERWRITE_ELEMENT, PythonDevice,
                          SLOT_ELEMENT, STRING_ELEMENT, UINT32_ELEMENT)
from karabo.common.scenemodel.api import write_scene
from karabo.common.states import State
from karabo.middlelayer import read_project_model
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
            OVERWRITE_ELEMENT(expected).key("state")
            .setNewOptions(State.ERROR, State.ON, State.INIT)
            .setNewDefaultValue(State.INIT)
            .commit(),
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
        self.registerSlot(self.slotListItems)
        self.registerSlot(self.slotListDomains)
        self.registerSlot(self.slotUpdateAttribute)
        self.registerSlot(self.slotGetScene)
        self.registerInitialFunction(self.initialization)
        self.user_db_sessions = {}

    def initialization(self):
        """
        Initialization function of this device. Checks that the configured
        database is indeed reachable and accessible

        If the database is reachable updates the device state to ON,
        otherwise brings the device into ERROR
        """
        # check if we can connect to the database
        try:
            assure_running(self.get("host"), self.get("port"))
            self.updateState(State.ON)
        except ProjectDBError as e:
            self.log.ERROR("ProjectDBError : {}".format(str(e)))
            self.updateState(State.ERROR)

    def allowLock(self):
        # this service device cannot be locked
        return False

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

    def slotGetScene(self, params):
        """Request a scene directly from the database in the correct format

        This protocol is in the style of the capability protocol and expects
        a Hash with parameters:

            name: Name of scene --optional
            domain: Domain to look for the scene item
            uuid: UUID of the scene item
            db_token: db token of the running instance

        :param params: A `Hash` containing the method parameters
        """
        self.log.DEBUG('Requesting scene directly from database!')

        name = params.get('name', default='')
        token = params.get('db_token')
        domain = params.get('domain')
        uuid = [params.get('uuid')]

        # Check if we are already initialized!
        self._checkDbInitialized(token)

        # Start filling the payload Hash
        # ----------------------------------------
        payload = Hash('success', False)
        payload.set('name', name)
        with self.user_db_sessions[token] as db_session:
            try:
                item = db_session.load_item(domain, uuid)[0]
                xml = item['xml']
                item_type = etree.fromstring(xml).get('item_type')
                if item_type == 'scene':
                    scene = read_project_model(StringIO(xml))
                    payload.set('data', write_scene(scene))
                    payload.set('success', True)
            except ProjectDBError as e:
                self.log.INFO('ProjectDBError in directly loading database '
                              'scene: {}'.format(e))

        self.reply(Hash('type', 'deviceScene',
                        'origin', self.getInstanceId(),
                        'payload', payload))

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
                      - overwrite: behavior in case of conflict
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
                # Remove XML data to not send it back
                item['xml'] = ''
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
            keys = [it.get('uuid') for it in items
                    if it.get('domain') == domain]
            assert len(keys) == len(items), "Incorrect domain given!"

            try:
                items = db_session.load_item(domain, keys)
                for item in items:
                    uuid = item["uuid"]
                    h = Hash("domain", domain,
                             "uuid", uuid,
                             "xml", item["xml"])
                    loadedItems.append(h)
                    # Remove from the list of requested keys
                    keys.remove(uuid)

                # Any keys left were not in the database
                if len(keys) > 0:
                    success = False
                    exceptionReason = 'Items "{}" not found!'.format(keys)

            except ProjectDBError as e:
                exceptionReason = str(e)
                success = False
        self.reply(Hash('items', loadedItems,
                        'success', success,
                        'reason', exceptionReason))

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
                    h = Hash('uuid', r['uuid'],
                             'item_type', r['item_type'],
                             'simple_name', r['simple_name'],
                             'is_trashed',  r['is_trashed'],
                             'date', r['date'],
                             'user', r['user'])
                    h.set('description', r['description'])
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

    def slotUpdateAttribute(self, token, items):
        """
        Update any attribute of given ``items`` in the database

        :param token: database user token
        :param items: list of Hashes containing information on which items
                      to update. Each list entry should be a Hash containing

                      - domain: domain the item resides at
                      - uuid: the uuid of the item
                      - item_type: indicate type of item which attribute should
                                   be changed
                      - attr_name: name of attribute which should be changed
                      - attr_value: value of attribute which should be changed

        :return: a list of Hashes where each entry has keys: domain, item_type,
                 uuid, attr_name, attr_value
        """

        self._checkDbInitialized(token)

        with self.user_db_sessions[token] as db_session:
            exceptionReason = ""
            success = True
            resHashes = []
            try:
                res = db_session.update_attributes(items)
                for r in res:
                    h = Hash('domain', r['domain'],
                             'item_type', r['item_type'],
                             'uuid', r['uuid'],
                             'attr_name', r['attr_name'],
                             'attr_value', r['attr_value'])
                    resHashes.append(h)
            except ProjectDBError as e:
                exceptionReason = str(e)
                success = False
            self.reply(Hash('items', resHashes,
                            'success', success,
                            'reason', exceptionReason))
