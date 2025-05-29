# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from io import StringIO

from lxml import etree

from karabo.common.project.api import (
    PROJECT_DB_TYPE_DEVICE_CONFIG, PROJECT_DB_TYPE_DEVICE_INSTANCE,
    PROJECT_DB_TYPE_DEVICE_SERVER, PROJECT_DB_TYPE_MACRO,
    PROJECT_DB_TYPE_PROJECT, PROJECT_DB_TYPE_SCENE)
from karabo.common.scenemodel.api import write_scene
from karabo.common.states import State
from karabo.middlelayer import (
    AccessLevel, AccessMode, Bool, Configurable, Device, Hash, HashList, Node,
    Overwrite, Slot, String, TypeHash, VectorString, dictToHash, slot)
from karabo.middlelayer.signalslot import Signal
from karabo.native import read_project_model
from karabo.project_db import (
    ExistDbNode, LocalNode, ProjectDBError, RemoteNode)

_ITEM_TYPES = (
    PROJECT_DB_TYPE_DEVICE_CONFIG,
    PROJECT_DB_TYPE_DEVICE_INSTANCE,
    PROJECT_DB_TYPE_DEVICE_SERVER,
    PROJECT_DB_TYPE_MACRO,
    PROJECT_DB_TYPE_PROJECT,
    PROJECT_DB_TYPE_SCENE
)


def get_project():
    class ProjectNode(Configurable):

        protocol = String(
            defaultValue="exist_db",
            options=["exist_db", "remote", "local"],
            accessMode=AccessMode.INITONLY)

        testMode = Bool(
            displayedName="Test Mode",
            defaultValue=False,
            requiredAccessLevel=AccessLevel.EXPERT)

        locals()["exist_db"] = Node(ExistDbNode)
        locals()["remote"] = Node(RemoteNode)
        locals()["local"] = Node(LocalNode)

    return Node(ProjectNode)


class ProjectManager(Device):

    state = Overwrite(
        defaultValue=State.INIT,
        options=[State.ERROR, State.ON, State.INIT])

    projectDB = get_project()

    domainList = VectorString(
        displayedName="Domain List",
        defaultValue=[],
        description="List of allowed project DB domains. "
                    "Empty list means no restrictions.",
        accessMode=AccessMode.INITONLY)

    signalProjectUpdate = Signal(TypeHash(), String())

    def __init__(self, configuration):
        super().__init__(configuration)
        self.db_handle = None

    async def onInitialization(self):
        """
        Initialization function of this device. Checks that the configured
        database is indeed reachable and accessible

        If the database is reachable updates the device state to ON,
        otherwise brings the device into ERROR
        """
        try:
            self.db_handle = await self.get_project_db(
                init_db=True)
            self.state = State.ON
        except ProjectDBError as e:
            self.logger.error(f"ProjectDBError : {str(e)}")
            self.state = State.ERROR

    async def get_project_db(self, init_db=False):
        """Internal helper to get the project db"""
        test_mode = self.projectDB.testMode.value
        node = self.projectDB.protocol
        db_node = getattr(self.projectDB, node)
        db = await db_node.get_db(test_mode, init_db=init_db)
        if init_db:
            self.logger.info("Project DB initialized ...")
        return db

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
        return self.host.value, self.port.value

    @slot
    async def slotGetScene(self, params):
        """Request a scene directly from the database in the correct format

        This protocol is in the style of the capability protocol and expects
        a Hash with parameters:

            name: Name of scene --optional
            domain: Domain to look for the scene item
            uuid: UUID of the scene item

        :param params: A `Hash` containing the method parameters
        """
        self.logger.debug('Requesting scene directly from database!')

        name = params.get('name', default='')
        # Note: Token used for Karabo >=2.13
        domain = params.get('domain')
        uuid = [params.get('uuid')]

        # Start filling the payload Hash
        # ----------------------------------------
        payload = Hash('success', False)
        payload.set('name', name)
        async with self.db_handle as db_session:
            try:
                items = await db_session.load_item(domain, uuid)
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
    async def slotBeginUserSession(self, token: str):
        """
        Initialize a DB connection for a user
        :param token: database user token
        """
        return Hash("success", True)

    @slot
    def slotEndUserSession(self, token: str):
        """
        End a user session
        :param token: database user token
        """
        return Hash("success", True)

    async def _save_items(self, items):
        """Internally used method to store items in the project database"""
        saved_items = []
        project_uuids = []
        async with self.db_handle as db_session:
            for item in items:
                xml = item.get("xml")
                # Remove XML data to not send it back
                item['xml'] = ''
                uuid = item.get("uuid")
                # XXX: be backward compatible (<2.8.0)!
                item_type = item.get("item_type", "unknown")
                if item_type == "project":
                    project_uuids.append(uuid)
                domain = item.get("domain")
                exceptionReason = ""
                success = True
                meta = None
                # All items have their individual success bool
                try:
                    meta = await db_session.save_item(
                        domain, uuid, xml, True)
                    meta = dictToHash(meta)
                except ProjectDBError as e:
                    success = False
                    exceptionReason = str(e)
                item.set("success", success)
                item.set("reason", exceptionReason)
                item.set("entry", meta)
                saved_items.append(item)

        return saved_items, project_uuids

    @slot
    async def slotGenericRequest(self, params):
        """Implements a generic Hash-in/Hash-out interface

        :param params: the input Hash.
            it must contain:
            - a `type` string matching a non-generic slot name
            - the optional fields required by the non-generic slot
            requested.
        """
        self.logger.info(f"Generic request: {params.get('type')}")
        msg = "Input must be a Hash"
        assert isinstance(params, Hash), msg
        msg = "'type' must be present in the input hash"
        assert "type" in params, msg

        action_type = params["type"]
        if action_type == "listItems":
            return await self.slotListItems(
                params['domain'],
                params.get('item_types', None))
        elif action_type == "loadItems":
            return await self.slotLoadItems(params['items'])
        elif action_type == "listDomains":
            return await self.slotListDomains()
        elif action_type == "updateTrashed":
            return await self.slotUpdateTrashed(params)
        elif action_type == "saveItems":
            return await self.slotSaveItems(
                params['items'], params.get('client', None))
        elif action_type == "beginUserSession":
            return await self.slotBeginUserSession(None)
        elif action_type == "endUserSession":
            return self.slotEndUserSession(None)
        elif action_type == "listProjectsWithDevice":
            return await self.slotListProjectsWithDevice(params)
        elif action_type == "listProjectsWithMacro":
            return await self.slotListProjectsWithMacro(params)
        elif action_type == "listProjectsWithServer":
            return await self.slotListProjectsWithServer(params)
        elif action_type == "listDomainWithDevices":
            return await self.listDomainWithDevices(params)
        raise NotImplementedError(f"{type} not implemented")

    @slot
    async def slotSaveItems(self, items, client=None):
        """Save items in project database

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
        saved, uuids = await self._save_items(items)
        if client and uuids:
            self.signalProjectUpdate(Hash("projects", uuids,
                                          "client", client),
                                     self.deviceId)

        return Hash('items', saved)

    @slot
    async def slotLoadItems(self, items):
        """
        Loads items from the database

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
        loadedItems = []
        async with self.db_handle as db_session:
            # verify that items belong to single domain
            domain = items[0].get("domain")
            keys = [it.get('uuid') for it in items
                    if it.get('domain') == domain]
            assert len(keys) == len(items), "Incorrect domain given!"

            items = await db_session.load_item(domain, keys)
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
                raise ProjectDBError(f'Items "{keys}" not found!')

        return Hash('items', loadedItems)

    @slot
    async def slotListItems(self, domain: str, item_types: list | None = None):
        """
        List items in domain which match item_types if given, or all items
        if not given

        :param domain: domain to list items from
        :param item_types: list or tuple of item_types to list
        :return: a list of Hashes where each entry has keys: uuid, item_type
            and simple_name
        """
        if item_types is None:
            item_types = _ITEM_TYPES
        async with self.db_handle as db_session:
            hl = []
            res = await db_session.list_items(domain, item_types)
            for r in res:
                item_type = r['item_type']
                h = Hash('uuid', r['uuid'],
                         'item_type', item_type,
                         'simple_name', r['simple_name'],
                         'date', r['date'])
                if item_type == PROJECT_DB_TYPE_PROJECT:
                    h["is_trashed"] = r["is_trashed"]
                hl.append(h)
        return Hash('items', hl)

    @slot
    async def slotListNamedItems(self, domain, item_type, simple_name):
        """
        List items in domain which match item_type and simple_name

        :param domain: domain to list items from
        :param item_type: item_type to match
        :param simple_name: simple_name to match
        :return: a list of Hashes where each entry has keys: uuid, date,
            item_type and simple_name sorted by date
        """
        async with self.db_handle as db_session:
            hl = []
            res = await db_session.list_named_items(
                domain, item_type, simple_name)
            for r in res:
                h = Hash('uuid', r['uuid'],
                         'item_type', r['item_type'],
                         'simple_name', r['simple_name'],
                         'is_trashed', r['is_trashed'],
                         'date', r['date'])
                hl.append(h)
            hl.sort(key=lambda x: x['date'])
        return Hash('items', hl)

    @slot
    async def listDomainWithDevices(self, info: Hash):
        """List devices available in a domain"""
        domain = info["domain"]
        async with self.db_handle as db_session:
            res = HashList()
            topology = await db_session.get_devices_from_domain(domain)
            res = HashList([Hash(device) for device in topology])
            return Hash('items', res)

    @slot
    async def slotListDomains(self):
        """
        List domains available on this database

        :return:
        """
        async with self.db_handle as db_session:
            res = await db_session.list_domains()
            if len(self.domainList):
                res = [domain for domain in res
                       if domain in self.domainList]
            return Hash('domains', res)

    async def _slot_list_projects(self, call: str, domain: str, name: str):
        """Generic slot logic for fetching and formatting projects."""
        async with self.db_handle as db_session:
            method = getattr(db_session, call)
            projects = await method(domain, name)
            result = HashList(
                [Hash("project_name", p["project_name"],
                      "uuid", p["uuid"],
                      "date", p["date"],
                      "items", p["items"])
                 for p in projects])
        return Hash("items", result)

    @slot
    async def slotListProjectsWithDevice(self, args: Hash):
        """
        List projects in domain which have configurations for a given device.

        :param args: a hash that must contain the keys , "domain" and
            "name" with the following meanings:
            "domain" is the domain to list

        :return: a Hash with key, "projects", with a list of Hashes for its
            value. Each Hash in the list has four keys:
                - "uuid",
                - "project_name",
                - "date"
                - "items"
        """
        return await self._slot_list_projects(
            "get_projects_with_device", args["domain"], args["name"])

    @slot
    async def slotListProjectsWithMacro(self, args: Hash):
        """
        List projects in domain which have macros.

        :param args: a hash that must contain the keys
            "name" with the following meanings:
            "domain" is the domain to list

        :return: a Hash with key, "projects", with a list of Hashes for its
            value. Each Hash in the list has four keys:
                - "uuid",
                - "project_name",
                - "date"
                - "items"
        """
        return await self._slot_list_projects(
            "get_projects_with_macro", args["domain"], args["name"])

    @slot
    async def slotListProjectsWithServer(self, args: Hash):
        """
        List projects in domain which have server.

        :param args: a hash that must contain the keys
            "name" with the following meanings:
            "domain" is the domain to list

        :return: a Hash with key, "projects", with a list of Hashes for its
            value. Each Hash in the list has four keys:
                - "uuid",
                - "project_name",
                - "date"
                - "items"
        """
        return await self._slot_list_projects(
            "get_projects_with_server", args["domain"], args["name"])

    @slot
    async def slotUpdateTrashed(self, info) -> Hash:
        """
        :returns: Hash with domain
        """
        item = info["items"]
        async with self.db_handle as db_session:
            domain = item["domain"]
            await db_session.update_trashed(**item)

        return Hash("domain", domain)

    @slot
    async def slotListProjectAndConfForDevice(self, domain, deviceId):
        async with self.db_handle as db_session:
            hl = []
            res = await db_session.get_projects_with_conf(
                domain, deviceId)
            hl = [Hash("project_name", k, "active_config_ref", v)
                  for k, v in res.items()]
            return Hash('items', hl)
