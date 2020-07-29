from io import StringIO
from lxml import etree

from karabo.common.scenemodel.api import write_scene
from karabo.common.states import State
from karabo.middlelayer import (
    AccessLevel, AccessMode, Bool, Device, Hash, HashType, Overwrite, slot,
    Slot, String, UInt32, VectorString)
from karabo.middlelayer_api.signalslot import Signal
from karabo.native import read_project_model
from karabo.project_db.project_database import ProjectDatabase
from karabo.project_db.util import get_db_credentials, ProjectDBError


def dictToHash(d):
    h = Hash()
    for k, v in d.items():
        if isinstance(v, dict):
            h[k] = dictToHash(v)
        elif isinstance(v, (list, tuple)):
            if len(v) > 0 and isinstance(v[0], dict):
                h[k] = [dictToHash(vv) for vv in v]
            else:
                h[k] = v
        else:
            h[k] = v
    return h


class ProjectManager(Device):

    # As long as part of Karabo framework, just inherit __version__ from Device

    state = Overwrite(
        defaultValue=State.INIT,
        options=[State.ERROR, State.ON, State.INIT])

    visibility = Overwrite(
        defaultValue=AccessLevel.ADMIN,
        options=[AccessLevel.ADMIN])

    host = String(
        defaultValue="localhost",
        displayedName="Database host",
        requiredAccessLevel=AccessLevel.EXPERT)

    port = UInt32(
        displayedName="Port",
        defaultValue=8080,
        requiredAccessLevel=AccessLevel.EXPERT)

    testMode = Bool(
        displayedName="Test Mode",
        defaultValue=False,
        requiredAccessLevel=AccessLevel.ADMIN)

    domainList = VectorString(
        displayedName="Domain List",
        defaultValue=[],
        description="List of allowed project DB domains. Empty list means no "
                    "restrictions.",
        accessMode=AccessMode.INITONLY)

    signalProjectUpdate = Signal(HashType(), String())

    def __init__(self, configuration):
        super(ProjectManager, self).__init__(configuration)
        self.user_db_sessions = {}

    async def onInitialization(self):
        """
        Initialization function of this device. Checks that the configured
        database is indeed reachable and accessible

        If the database is reachable updates the device state to ON,
        otherwise brings the device into ERROR
        """
        try:
            # check if we can connect to the database
            host, port = self._getCurrentConfig()
            user, password = get_db_credentials(self.testMode.value)
            with ProjectDatabase(user, password,
                                 server=host, port=port,
                                 test_mode=self.testMode.value,
                                 init_db=True):
                self.state = State.ON
        except ProjectDBError as e:
            self.logger.error("ProjectDBError : {}".format(str(e)))
            self.state = State.ERROR

    @Slot(displayedName="Reset", allowedStates=[State.ERROR],
          requiredAccessLevel=AccessLevel.EXPERT)
    async def reset(self):
        """Resetting the device brings it into `State.INIT`
        """
        self.state = State.INIT
        await self.onInitialization()

    def _getCurrentConfig(self):
        """
        Return database relevant configuration from expected parameters
        as tuple
        :return: at tuple of host, port
        """
        return self.host.value, int(self.port.value)

    @slot
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
        self.logger.debug('Requesting scene directly from database!')

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
                items = db_session.load_item(domain, uuid)
                for item in items:
                    xml = item['xml']
                    item_type = etree.fromstring(xml).get('item_type')
                    if item_type == 'scene':
                        scene = read_project_model(StringIO(xml))
                        payload.set('data', write_scene(scene))
                        payload.set('success', True)
            except ProjectDBError as e:
                self.logger.debug('ProjectDBError in directly loading '
                                  'database scene: {}'.format(e))

        return Hash('type', 'deviceScene',
                    'origin', self.deviceId,
                    'payload', payload)

    @slot
    def slotBeginUserSession(self, token):
        """
        Initialize a DB connection for a user
        :param token: database user token
        """
        # XXX: Leave these hardcoded until session tokens are working
        host, port = self._getCurrentConfig()
        user, password = get_db_credentials(self.testMode.value)
        db = ProjectDatabase(user, password,
                             server=host,
                             port=port,
                             test_mode=self.testMode.value)
        self.user_db_sessions[token] = db

        self.logger.debug("Initialized user session")
        return Hash("success", True)

    @slot
    def slotEndUserSession(self, token):
        """
        End a user session
        :param token: database user token
        """
        del self.user_db_sessions[token]

        self.logger.debug("Ended user session")
        return Hash("success", True)

    def _checkDbInitialized(self, token):
        if token not in self.user_db_sessions:
            raise RuntimeError("You need to init a database session first")

    def _save_items(self, token, items):
        """Internally used method to store items in the project database"""
        savedItems = []
        projectUuids = []
        with self.user_db_sessions[token] as db_session:
            for item in items:
                xml = item.get("xml")
                # Remove XML data to not send it back
                item['xml'] = ''
                uuid = item.get("uuid")
                # XXX: be backward compatible (<2.8.0)!
                item_type = item.get("item_type", "unknown")
                if item_type == "project":
                    projectUuids.append(uuid)
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

        return savedItems, projectUuids

    @slot
    def slotSaveItems(self, token, items, client=None):
        """Save items in project database

        :param token: database user token
        :param items: items to be save. Should be a list(Hash) object were each
                      entry is of the form:

                      - xml: xml of item
                      - uuid: uuid of item
                      - overwrite: behavior in case of conflict
                      - domain: to write to
        :param client: the client information (string) if provided

        :raises: `ProjectDBError` in case of database (connection) problems
                 `TypeError` in case an no type information or an unknown type
                 is found in the root element.
                 `RuntimeError` if no database if connected.
        """

        self.logger.debug("Saving items: {}".format([i.get("uuid") for i in
                                                     items]))
        self._checkDbInitialized(token)
        saved, uuids = self._save_items(token, items)
        if client and uuids:
            self.signalProjectUpdate(Hash("projects", uuids,
                                          "client", client),
                                     self.deviceId)

        return Hash('items', saved)

    @slot
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

        self.logger.debug("Loading items: {}"
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
        return Hash('items', loadedItems,
                    'success', success,
                    'reason', exceptionReason)

    @slot
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
                             'is_trashed', r['is_trashed'],
                             'date', r['date'],
                             'user', r['user'])
                    h.set('description', r['description'])
                    resHashes.append(h)
            except ProjectDBError as e:
                exceptionReason = str(e)
                success = False
        return Hash('items', resHashes,
                    'success', success,
                    'reason', exceptionReason)

    @slot
    def slotListNamedItems(self, token, domain, item_type, simple_name):
        """
        List items in domain which match item_type and simple_name

        :param token: database user token
        :param domain: domain to list items from
        :param item_type: item_type to match
        :param simple_name: simple_name to match
        :return: a list of Hashes where each entry has keys: uuid, date,
                 item_type and simple_name sorted by date
        """
        self._checkDbInitialized(token)

        with self.user_db_sessions[token] as db_session:
            exceptionReason = ""
            success = True
            resHashes = []
            try:
                res = db_session.list_named_items(
                    domain, item_type, simple_name)
                for r in res:
                    h = Hash('uuid', r['uuid'],
                             'item_type', r['item_type'],
                             'simple_name', r['simple_name'],
                             'is_trashed', r['is_trashed'],
                             'date', r['date'],
                             'user', r['user'])
                    h.set('description', r['description'])
                    resHashes.append(h)
                resHashes.sort(key=lambda x: x['date'])
            except ProjectDBError as e:
                exceptionReason = str(e)
                success = False
        return Hash('items', resHashes,
                    'success', success,
                    'reason', exceptionReason)

    @slot
    def slotListDomains(self, token):
        """
        List domains available on this database
        :param token: database user token
        :return:
        """
        self._checkDbInitialized(token)

        with self.user_db_sessions[token] as db_session:
            res = db_session.list_domains()
            if len(self.domainList):
                res = [domain for domain in res
                       if domain in self.domainList]
            return Hash('domains', res)

    @slot
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
            return Hash('items', resHashes,
                        'success', success,
                        'reason', exceptionReason)

    @slot
    def slotListProjectAndConfForDevice(self, token, domain, deviceId):
        self._checkDbInitialized(token)

        with self.user_db_sessions[token] as db_session:
            exceptionReason = ""
            success = True
            resHashes = []
            try:
                res = db_session.get_projects_with_conf(domain, deviceId)
                resHashes = [Hash("project_name", k, "active_config_ref", v)
                             for k, v in res.items()]
            except ProjectDBError as e:
                exceptionReason = str(e)
                success = False
            return Hash('items', resHashes,
                        'success', success,
                        'reason', exceptionReason)
