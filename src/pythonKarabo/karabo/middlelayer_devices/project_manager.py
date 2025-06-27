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
from asyncio import wait_for
from io import StringIO

from lxml import etree

from karabo.common.project.api import (
    PROJECT_DB_TYPE_DEVICE_CONFIG, PROJECT_DB_TYPE_DEVICE_INSTANCE,
    PROJECT_DB_TYPE_DEVICE_SERVER, PROJECT_DB_TYPE_MACRO,
    PROJECT_DB_TYPE_PROJECT, PROJECT_DB_TYPE_SCENE)
from karabo.common.scenemodel.api import write_scene
from karabo.common.states import State
from karabo.middlelayer import (
    AccessLevel, AccessMode, Bool, Configurable, Device, Hash, HashList,
    KaraboError, Node, Overwrite, Signal, Slot, String, TypeHash, VectorString,
    decodeXML, instantiate, slot)
from karabo.native import read_project_model
from karabo.project_db import (
    ExistDbNode, LocalNode, ProjectDBError, RemoteNode)

_ITEM_TYPES = [
    PROJECT_DB_TYPE_DEVICE_CONFIG,
    PROJECT_DB_TYPE_DEVICE_INSTANCE,
    PROJECT_DB_TYPE_DEVICE_SERVER,
    PROJECT_DB_TYPE_MACRO,
    PROJECT_DB_TYPE_PROJECT,
    PROJECT_DB_TYPE_SCENE
]

_UNKNOWN_CLIENT = "__none__"


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
    async def slotGetScene(self, info):
        """Request a scene directly from the database in the correct format

        This protocol is in the style of the capability protocol and expects
        a Hash with parameters:

            name: Name of scene --optional
            domain: Domain to look for the scene item
            uuid: UUID of the scene item

        :param info: A `Hash` containing the method parameters
        """
        self.logger.debug('Requesting scene directly from database!')

        name = info.get('name', default='')
        domain = info.get('domain')
        uuid = [info.get('uuid')]

        payload = Hash('success', False)
        payload.set('name', name)
        async with self.db_handle as db_session:
            items = await db_session.load_item(domain, uuid)
            for item in items:
                xml = item['xml']
                item_type = etree.fromstring(xml).get('item_type')
                if item_type == 'scene':
                    scene = read_project_model(StringIO(xml))
                    payload.set('data', write_scene(scene))
                    payload.set('success', True)

        return Hash('type', 'deviceScene',
                    'origin', self.deviceId,
                    'payload', payload)

    async def _project_db_schema(self) -> int:
        async with self.db_handle as db_session:
            return await db_session.schema_version()

    async def _save_items(self, items):
        """Internally used method to store items in the project database"""
        saved_items = []
        project_uuids = []
        async with self.db_handle as db_session:
            for item in items:
                xml = item["xml"]
                uuid = item["uuid"]

                # XXX: be backward compatible (<2.8.0)!
                item_type = item.get("item_type", "unknown")
                if item_type == "project":
                    project_uuids.append(uuid)

                domain = item["domain"]
                reason = ""
                date = ""
                success = True
                # All items have their individual success bool
                try:
                    date = await db_session.save_item(
                        domain, uuid, xml, True)
                except ProjectDBError as e:
                    success = False
                    reason = str(e)

                ret = Hash(
                    "success", success,
                    "reason", reason,
                    "domain", domain,
                    "date", date,
                    "uuid", uuid)
                saved_items.append(ret)

        return saved_items, project_uuids

    @slot
    async def slotGenericRequest(self, info):
        """Implements a generic Hash-in/Hash-out interface

        :param info: the input Hash.
            it must contain:
            - a `type` string matching a non-generic slot name
            - the optional fields required by the non-generic slot
            requested.
        """
        self.logger.info(f"Generic request: {info.get('type')}")
        msg = "Input must be a Hash"
        assert isinstance(info, Hash), msg
        msg = "'type' must be present in the input hash"
        assert "type" in info, msg

        action_type = info["type"]
        if action_type == "listItems":
            return await self.slotListItems(info)
        elif action_type == "loadItems":
            return await self.slotLoadItems(info)
        elif action_type == "listDomains":
            return await self.slotListDomains()
        elif action_type == "updateTrashed":
            return await self.slotUpdateTrashed(info)
        elif action_type == "saveItems":
            return await self.slotSaveItems(info)
        elif action_type == "listProjectsWithDevice":
            return await self.slotListProjectsWithDevice(info)
        elif action_type == "listProjectsWithMacro":
            return await self.slotListProjectsWithMacro(info)
        elif action_type == "listProjectsWithServer":
            return await self.slotListProjectsWithServer(info)
        elif action_type == "listDomainWithDevices":
            return await self.listDomainWithDevices(info)
        elif action_type == "instantiateProjectDevice":
            return await self.instantiateProjectDevice(info)
        raise NotImplementedError(f"{action_type} not implemented")

    @slot
    async def slotSaveItems(self, info: Hash):
        """Save items in project database

        :param info: Hash with

            items: list(Hash) object were each entry is of the form:
                - xml: xml of item
                - uuid: uuid of item
                - domain: to write to
            client: the client information (string) if provided
            schema_version: the project db schema_version

        :raises: `ProjectDBError` in case of database (connection) problems
            `TypeError` in case an no type information or an unknown type
            is found in the root element.
            `RuntimeError` if no database if connected.
        """
        items = info["items"]
        self.logger.debug(
            "Saving items: {}".format([i.get("uuid") for i in items]))
        client = info.get("client", _UNKNOWN_CLIENT)

        schema_version = info.get("schema_version", 1)
        project_db_schema = await self._project_db_schema()
        if not schema_version == project_db_schema:
            text = ("Cannot store into project database "
                    f"with db schema {project_db_schema}.")
            raise ProjectDBError(text)

        saved, uuids = await self._save_items(items)
        if uuids:
            self.signalProjectUpdate(
                Hash("projects", uuids, "client", client),
                self.deviceId)

        return Hash('items', saved)

    @slot
    async def slotLoadItems(self, info: Hash):
        """Loads items from the database

        :param info: The input Hash with keys

            - `items` list of Hashes containing information on which items
                      to load. Each list entry should be a Hash containing
                      - uuid: the uuid of the item
                      - domain: domain to load item from

        :return: a Hash where the keys are the item uuids and values are the
            item XML. If the load failed the value for this uuid is set
            to False
        """
        items = info['items']
        self.logger.debug("Loading items: {}"
                          .format([i.get("uuid") for i in items]))
        loaded_items = []
        async with self.db_handle as db_session:
            # verify that items belong to single domain
            domain = items[0]["domain"]
            keys = [it['uuid'] for it in items
                    if it['domain'] == domain]
            assert len(keys) == len(items), "Incorrect domain given!"

            items = await db_session.load_item(domain, keys)
            for item in items:
                uuid = item["uuid"]
                h = Hash("domain", domain,
                         "uuid", uuid,
                         "xml", item["xml"])
                loaded_items.append(h)
                # Remove from the list of requested keys
                keys.remove(uuid)

            # Any keys left were not in the database
            if len(keys) > 0:
                raise ProjectDBError(f'Items "{keys}" not found!')

        return Hash('items', loaded_items)

    @slot
    async def slotListItems(self, info: Hash):
        """List items in domain which match item_types if given, or all items
        if not given

        :param info: The input Hash with keys
                       - domain: domain to list items from
                       - item_types: optional list of item_types

        :return: Hash with keys:

                items: list of Hashes where each entry has keys: uuid,
                       item_type and simple_name
        """
        domain = info['domain']
        item_types = info.get('item_types', _ITEM_TYPES)
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
    async def slotListNamedItems(self, info: Hash):
        """
        List items in domain which match item_type and simple_name

        :param info: Input Hash with keys:
                    - domain: domain to list items from
                    - item_type: item_type to match
                    - simple_name: simple_name to match

        :return: Hash with keys:

                items: a list of Hashes where each entry has keys: uuid, date,
                       item_type and simple_name sorted by date
        """
        domain = info["domain"]
        item_type = info["item_type"]
        simple_name = info["simple_name"]
        async with self.db_handle as db_session:
            hl = []
            res = await db_session.list_named_items(
                domain, item_type, simple_name)
            for r in res:
                item_type = r['item_type']
                h = Hash('uuid', r['uuid'],
                         'item_type', item_type,
                         'simple_name', r['simple_name'],
                         'date', r['date'])
                if item_type == PROJECT_DB_TYPE_PROJECT:
                    h['is_trashed'] = r['is_trashed']
                hl.append(h)
            hl.sort(key=lambda x: x['date'])
        return Hash('items', hl)

    @slot
    async def listDomainWithDevices(self, info: Hash):
        """List devices available in a domain"""
        domain = info["domain"]
        async with self.db_handle as db_session:
            topology = await db_session.get_devices_from_domain(domain)
            res = HashList([Hash(device) for device in topology])
            return Hash('items', res)

    @slot
    async def instantiateProjectDevice(self, info: Hash):
        """Instantiate a device on a server with classId"""
        deviceId = info["deviceId"]
        classId = info["classId"]
        serverId = info["serverId"]
        uuid = info["device_uuid"]
        async with self.db_handle as db_session:
            config = await db_session.get_device_config_from_device_uuid(
                uuid)
            if config is None:
                raise KaraboError(
                    f"No configuration found for deviceId {deviceId} "
                    f"with classId {classId}.")
            config = decodeXML(config)
            # gracefully the first (and only) key value pair, which is
            # the classId
            assert len(config) == 1
            key = next(iter(config), None)
            configuration = config[key]
            await wait_for(instantiate(
                serverId=serverId,
                classId=classId,
                deviceId=deviceId,
                configuration=configuration), timeout=10)
        return Hash()

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
    async def slotListProjectsWithDevice(self, info: Hash):
        """
        List projects in domain which have configurations for a given device.

        :param info: a hash that must contain the keys , "domain" and
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
            "get_projects_with_device", info["domain"], info["name"])

    @slot
    async def slotListProjectsWithMacro(self, info: Hash):
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
            "get_projects_with_macro", info["domain"], info["name"])

    @slot
    async def slotListProjectsWithServer(self, info: Hash):
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
            "get_projects_with_server", info["domain"], info["name"])

    @slot
    async def slotUpdateTrashed(self, info: Hash) -> Hash:
        """
        :returns: Hash with domain
        """
        item = info["items"]
        async with self.db_handle as db_session:
            domain = item["domain"]
            await db_session.update_trashed(**item)

        return Hash("domain", domain)

    @slot
    async def slotListProjectsWithDeviceConfigurations(
            self, info: Hash) -> Hash:
        domain = info["domain"]
        deviceId = info["deviceId"]
        async with self.db_handle as db_session:
            hl = []
            res = await db_session.get_projects_with_conf(
                domain, deviceId)
            hl = [Hash("project_name", k, "config_uuid", v)
                  for k, v in res.items()]
            return Hash('items', hl)
