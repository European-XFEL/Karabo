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
import datetime
from asyncio import gather
from pathlib import Path

from sqlmodel import SQLModel

from karabo.native import HashList

from ..bases import DatabaseBase
from ..util import ProjectDBError, make_str_if_needed, make_xml_if_needed
from .db_engine import create_local_engine, create_remote_engine
from .db_reader import DbReader
from .db_writer import DbWriter
from .models_xml import (
    emit_device_config_xml, emit_device_instance_xml, emit_device_server_xml,
    emit_macro_xml, emit_project_xml, emit_scene_xml)


class SQLDatabase(DatabaseBase):

    def __init__(self, user: str = "", password: str = "",
                 server: str = "", port: int = -1, db_name: str = "local.db",
                 local: bool = False, remove_orphans: bool = False):
        super().__init__()
        if local:
            self.db_engine, self.session_gen = create_local_engine(db_name)
        else:
            self.db_engine, self.session_gen = (
                create_remote_engine(user, password, server, port, db_name))
        self.dbName = db_name
        self.reader = DbReader(self.session_gen)
        self.writer = DbWriter(self.session_gen, remove_orphans)
        self.initialized = False
        self.local = local

    async def initialize(self):
        async with self.db_engine.begin() as conn:
            await conn.run_sync(SQLModel.metadata.create_all)
        self.initialized = True

    async def delete(self):
        """Deletes the database schema and file."""
        async with self.db_engine.begin() as conn:
            await conn.run_sync(SQLModel.metadata.drop_all)
        await self.db_engine.dispose()
        if self.local:
            path = Path(self.dbName)
            if path.is_file():
                path.unlink()

    async def __aenter__(self):
        if not self.initialized:
            raise RuntimeError("Database needs to be initialized")
        return self

    async def __aexit__(self, exc_type, exc_value, traceback):
        return await super().__aexit__(exc_type, exc_value, traceback)

    async def list_domains(self) -> list[str]:
        return await self.reader.list_domains()

    async def domain_exists(self, domain: str) -> bool:
        domains = await self.list_domains()
        return domain in domains

    async def add_domain(self, domain: str):
        if await self.domain_exists(domain):
            return
        await self.writer.add_domain(domain)

    async def list_items(
            self, domain: str,
            item_types: list[str] | tuple[str] | None = None) -> list[
                dict[str, any]]:
        """
        List items in domain which match item_types if given, or all items
        if not given
        :param domain: domain to list items from
        :param item_types: list or tuple of item_types to list
        :return: a list of dicts where each entry has keys: uuid, item_type
                 and simple_name
        """
        if not await self.domain_exists(domain):
            raise ProjectDBError(f'Domain "{domain}" not found')
        if item_types is None:
            item_types = ["project", "macro", "scene", "device_server",
                          "device_instance", "device_config"]
        result = []
        for item in item_types:
            match item:
                case "project":
                    projects = await self.reader.get_domain_projects(domain)
                    for project in projects:
                        result.append(project)
                case "macro":
                    macros = await self.reader.get_domain_macros(domain)
                    for macro in macros:
                        result.append(macro)
                case "scene":
                    scenes = await self.reader.get_domain_scenes(domain)
                    for scene in scenes:
                        result.append(scene)
                case "device_server":
                    servers = await self.reader.get_domain_device_servers(
                        domain)
                    for server in servers:
                        result.append(server)
                case "device_instance":
                    instances = await self.reader.get_domain_device_instances(
                        domain)
                    for instance in instances:
                        result.append(instance)
                case "device_config":
                    configs = await self.reader.get_domain_device_configs(
                        domain)
                    for config in configs:
                        result.append(config)
                case _:
                    raise ValueError(f'Unrecognized item_type, "{item}"')
        return result

    async def list_named_items(
        self, domain: str, item_type: str, simple_name: str
    ) -> list[dict[str, any]]:
        """
        List items in domain which match item_type and simple_name

        :param domain: domain to list items from
        :param item_type: item_type to match
        :param simple_name: name to match
        :return: a list of dicts where each entry has keys: uuid, item_type
                 and simple_name
        """
        items = await self.list_items(domain, [item_type])
        return [item for item in items
                if item.get('simple_name') == simple_name]

    async def _load_project_item(self, uuid: str) -> dict[str, any] | None:
        item = None
        project = await self.reader.get_project_from_uuid(uuid)
        if project:
            subprojects = await self.reader.get_subprojects_of_project(project)
            scenes = await self.reader.get_scenes_of_project(project)
            macros = await self.reader.get_macros_of_project(project)
            servers = await self.reader.get_device_servers_of_project(project)
            await self.writer.register_project_load(project)
            item = {
                "uuid": uuid,
                "xml": emit_project_xml(
                    project, scenes, macros, servers, subprojects)}
        return item

    async def _load_scene_item(self, uuid: str) -> dict[str, any] | None:
        item = None
        scene = await self.reader.get_scene_from_uuid(uuid)
        if scene:
            item = {
                "uuid": uuid,
                "xml": emit_scene_xml(scene)}
        return item

    async def _load_macro_item(self, uuid: str) -> dict[str, any] | None:
        item = None
        macro = await self.reader.get_macro_from_uuid(uuid)
        if macro:
            item = {
                "uuid": uuid,
                "xml": emit_macro_xml(macro)}
        return item

    async def _load_server_item(self, uuid: str) -> dict[str, any] | None:
        item = None
        server = await self.reader.get_device_server_from_uuid(uuid)
        if server:
            instances = await self.reader.get_device_instances_of_server(
                server)
            item = {
                "uuid": uuid,
                "xml": emit_device_server_xml(server, instances)}
        return item

    async def _load_instance_item(self, uuid: str) -> dict[str, any] | None:
        item = None
        instance = await self.reader.get_device_instance_from_uuid(uuid)
        if instance:
            configs = await self.reader.get_device_configs_of_instance(
                instance)
            item = {
                "uuid": uuid,
                "xml": emit_device_instance_xml(instance, configs)}
        return item

    async def _load_config_item(self, uuid: str) -> dict[str, any] | None:
        item = None
        config = await self.reader.get_device_config_from_uuid(uuid)
        if config:
            item = {
                "uuid": uuid,
                "xml": emit_device_config_xml(config)}
        return item

    async def _load_unknown_type(self, item_uuid: str):
        item = await self._load_project_item(item_uuid)
        if item is None:
            item = await self._load_scene_item(item_uuid)
        if item is None:
            item = await self._load_macro_item(item_uuid)
        if item is None:
            item = await self._load_server_item(item_uuid)
        if item is None:
            item = await self._load_instance_item(item_uuid)
        if item is None:
            item = await self._load_config_item(item_uuid)

        return item

    async def load_item(self, domain: str, items_uuids: list[str],
                        item_type: str | None = None) -> list[dict[str, any]]:
        """
        Loads a list of item of a specified type from a given domain.

        :param domain: the name of the domain from which the items should be
                       loaded - not used for SQL databases; legacy from
                       ExistDB
        :param item_uuid: a list with the UUIDs of the items to load
        :param item_type: the type of the items to be loaded
        :return: A list of dictionaries with keys "uuid" and "xml".
        :raises: ProjectDBError if an item corresponding to a given
                 (UUID, type) combination couldn't be found.'
        """
        futures = []
        for item_uuid in items_uuids:
            match item_type:
                case "project":
                    futures.append(self._load_project_item(item_uuid))
                case "macro":
                    futures.append(self._load_macro_item(item_uuid))
                case "scene":
                    futures.append(self._load_scene_item(item_uuid))
                case "device_server":
                    futures.append(self._load_server_item(item_uuid))
                case "device_instance":
                    futures.append(self._load_instance_item(item_uuid))
                case "device_config":
                    futures.append(self._load_config_item(item_uuid))
                case _:
                    # item_type not known; try exhaustively by UUID
                    futures.append(self._load_unknown_type(item_uuid))

        # XXX: Item none protection
        loaded_items = await gather(*futures)
        return loaded_items

    async def _check_for_modification(self, uuid: str, old_date: str,
                                      item_type: str) -> tuple[bool, str]:
        """ Check whether the item with of the given `domain` and `uuid` was
        modified

        :param uuid: the item's uuid
        :param old_date: the item's last modified time stamp
        :param item_type: the type of the item to be checked (e.g. 'project')
        :return: A tuple stating whether the item was modified inbetween and a
                 string describing the reason
        """
        current_modified = None
        match item_type:
            case "project":
                project = await self.reader.get_project_from_uuid(uuid)
                if project:
                    current_modified = project.date
            case "macro":
                macro = await self.reader.get_macro_from_uuid(uuid)
                if macro:
                    current_modified = macro.date
            case "scene":
                scene = await self.reader.get_scene_from_uuid(uuid)
                if scene:
                    current_modified = scene.date
            case "device_server":
                server = await self.reader.get_device_server_from_uuid(uuid)
                if server:
                    current_modified = server.date
            case "device_instance":
                instance = await self.reader.get_device_instance_from_uuid(
                    uuid)
                if instance:
                    current_modified = instance.date
            case "device_config":
                config = await self.reader.get_device_config_from_uuid(uuid)
                if config:
                    current_modified = config.date

        if current_modified:
            old_date_date = datetime.datetime.fromisoformat(old_date)
            # Make sure old_date_date has timezone information
            if old_date_date.tzinfo is None:
                old_date_date = old_date_date.replace(tzinfo=datetime.UTC)
            # current_modified was read from the DB as a naive datetime, but we
            # know it is in UTC. Add the timezone information so it can be
            # compared to the non-naive obtained from old_date.
            current_modified = current_modified.replace(
                tzinfo=datetime.UTC)
            if old_date_date < current_modified:
                # The data has been changed in the database between the
                # old_date that the GUI client sent and the current time.
                return (True,
                        f'Item of type "{item_type}" with uuid "{uuid}" '
                        f'has been modified in the DB ({current_modified}) '
                        'after its last retrieval by the client '
                        f'({old_date_date}).')

        return (False, "")

    async def save_item(self, domain: str, uuid: str, item_xml: str,
                        overwrite: bool = False) -> dict[str, any]:
        """
        Saves a item xml file into the database. It will
        create a new entry if the item does not exist yet, or update the item
        if it does exist.

        The root node of the xml should contain a `item_type` entry identifying
        the type of the item as one of the following:

        'projects', 'scenes', 'macros', 'device_configs', 'device_servers'

        If domain does not exist in the database a RuntimeError is raised.

        :param domain: the domain under which this item is to be stored
        :param uuid: the item's uuid
        :param item_xml: the xml containing the item information
        :param overwrite (unused): defaults to False. If set to True versioning
                                   information is removed prior to database
                                   injection, allowing to overwrite in case of
                                   versioning conflicts.

        :return: If saving was successful (no conflict), a dictionary with
                 the updated versioning information after saving the item.


        :raises: ProjectDBError on failure.
        """
        del overwrite  # To avoid unintentional use of unused parameter

        # Comply with the ExistDB back-end, which creates the domain for the
        # project being saved if the domain doesn't exist yet. The add_domain
        # method has the same "ensure domain exists" semantics of its overriden
        # counterpart in DatabaseBase.
        await self.add_domain(domain)

        # Extract some information
        try:
            # NOTE: The client might send us garbage
            item_tree = make_xml_if_needed(item_xml)
        except ValueError:
            msg = f'XML parse error for item "{uuid}"'
            raise ProjectDBError(msg)

        # XXX: Cannot be `None`
        item_type = item_tree.attrib['item_type']

        if 'user' not in item_tree.attrib:
            item_tree.attrib['user'] = 'Karabo User'
        if not item_tree.attrib.get('date'):
            timestamp = datetime.datetime.now(datetime.UTC).isoformat(
                timespec="seconds")
            item_tree.attrib['date'] = timestamp
        else:
            modified, reason = await self._check_for_modification(
                uuid, item_tree.attrib['date'], item_type)
            if modified:
                message = "The <b>{}</b> item <b>{}</b> could not be saved: " \
                          "{}".format(item_tree.attrib.get('item_type', ''),
                                      item_tree.attrib.get('simple_name', ''),
                                      reason)
                raise ProjectDBError(message)
            # Update time stamp
            timestamp = datetime.datetime.now(datetime.UTC).isoformat(
                timespec="seconds")
            item_tree.attrib['date'] = timestamp

        # XXX: Add a revision/alias to keep old code from blowing up
        # Is this needed?
        item_tree.attrib['revision'] = '0'
        item_tree.attrib['alias'] = 'default'

        # And back conversion to string
        item_xml = make_str_if_needed(item_tree)

        match item_type:
            case "project":
                await self.writer.save_project_item(
                    domain, uuid, item_xml, timestamp)
            case "macro":
                await self.writer.save_macro_item(
                    uuid, item_xml, timestamp)
            case "scene":
                await self.writer.save_scene_item(
                    uuid, item_xml, timestamp)
            case "device_server":
                await self.writer.save_device_server_item(
                    uuid, item_xml, timestamp)
            case "device_instance":
                await self.writer.save_device_instance_item(
                    uuid, item_xml, timestamp)
            case "device_config":
                await self.writer.save_device_config_item(
                    uuid, item_xml, timestamp)
            case _:
                raise ProjectDBError(
                    f"Saving of items of type '{item_type}' not supported")

        meta = {"domain": domain, "uuid": uuid, "date": timestamp}
        return meta

    async def get_configurations_from_device_name_part(
        self, domain: str, device_id_part: str,
        only_active: bool = False
    ) -> list[dict[str, str]]:
        """
        Returns a list of configurations for a given device

        :param domain: DB domain
        :param device_id_part: part of device name; search is case-insensitive.
        :param only_active: if True only returns the active configurations

        :return: a list of dicts:

            [{"config_id": uuid of the configuration,
              "device_uuid": device instance uuid in the DB,
              "device_id": device instance id},
              ...
            ]
        """
        instances = await self.reader.get_domain_device_instances_by_name_part(
            domain, device_id_part
        )

        results = []
        for instance in instances:
            configs = await self.reader.get_device_configs_of_instance(
                instance)
            filtered = [
                {
                    "config_id": config.uuid,
                    "device_uuid": instance.uuid,
                    "device_id": instance.name
                }
                for config in configs
                if config.is_active or not only_active
            ]
            results.extend(filtered)

        return results

    async def get_configurations_from_device_name(
        self, domain: str, instance_id: str
    ) -> list[dict[str, str]]:
        """
        Returns a list of active configurations for a given device_id

        :param domain: DB domain
        :param instance_id: instance id of the device
        :return: a list of dicts:
            [{"configid": uuid of the configuration,
              "instanceid": device instance uuid in the DB},
              ...
            ]
        """
        configs = await self.get_configurations_from_device_name_part(
            domain, instance_id, only_active=True)

        items = [
            {"configid": config["config_id"],
             "instanceid": config["device_uuid"]}
            for config in configs]
        return items

    async def get_projects_data_from_device(
            self, domain: str, uuid: str) -> list[dict[str, any]]:
        """
        Returns the project which contain a device instance with a given uuid

        :param domain: DB domain
        :param uuid: the uuid of the device instance from the database
        :return: a list containing project names, uuids and last modification
                 date.
        """
        instance = await self.reader.get_device_instance_from_uuid(uuid)
        # From the DB structure, a given device instance only belongs to
        # one project
        project = await self.reader.get_device_instance_project(instance)
        results = None
        if project:
            results = [
                {"projectname": project.name,
                 "date": project.date.strftime("%Y-%m-%d %H:%M:%S"),
                 "uuid": project.uuid}]
        return results

    async def get_projects_from_device(
            self, domain: str, uuid: str) -> set[str]:
        """
        Returns the projects which contain a device instance with a given uuid

        :param domain: DB domain
        :param uuid: the uuid of the device instance from the database
        :return: a set containing project names
        """
        projects = set()
        projects_datum = self.get_projects_data_from_device(domain, uuid)
        for project in projects_datum:
            projects.add(project['projectname'])
        return projects

    async def update_attributes(self, items: HashList) -> list[dict[str, any]]:
        """ Update attribute for the given ``items``

        :param items: list of Hashes containing information on which items
                      to update. Each list entry should be a Hash containing

                      - domain: domain the item resides at
                      - uuid: the uuid of the item
                      - item_type: indicate type of item which attribute should
                                   be changed
                      - attr_name: name of attribute which should be changed
                      - attr_value: value of attribute which should be changed

        :return: a list of dicts where each entry has keys: domain, uuid,
                 item_type, attr_name, attr_value

        :raises: ProjectDBError on failure
        """

        res_items = []
        for item in items:
            model: SQLModel | None = None
            item_uuid = item['uuid']
            item_type = item['item_type']
            attr_value = item['attr_value']
            # NOTE: The GUI client sends a bool as either "true" or "false"
            #       which are not accepted as valid values for a bool field
            #       by SQLModel (actually by SQLAlchemy). So those special
            #       cases are handled manually.
            if attr_value == "true":
                attr_value = True
            elif attr_value == "false":
                attr_value = False

            match item_type:
                case "project":
                    model = await self.reader.get_project_from_uuid(
                        item_uuid)
                case "device_server":
                    model = await self.reader.get_device_server_from_uuid(
                        item_uuid)
                case "device_instance":
                    model = await self.reader.get_device_instance_from_uuid(
                        item_uuid)
                case "device_config":
                    model = await self.reader.get_device_config_from_uuid(
                        item_uuid)
                case "macro":
                    model = await self.reader.get_macro_from_uuid(item_uuid)
                case "scene":
                    model = await self.reader.get_scene_from_uuid(item_uuid)
                case _:
                    raise ProjectDBError(
                        f"Unsupported item_type, '{item_type}', "
                        "for operation 'update_attributes'")

            if model is None:
                raise ProjectDBError(
                    f"No item of type '{item_type}' with UUID '{item_uuid}' "
                    "found in the database")

            setattr(model, item['attr_name'], attr_value)
            with self.session_gen() as session:
                session.add(model)
                session.commit()

            res_items.append(item)

        return res_items

    async def get_projects_with_conf(self, domain, device_id):
        """
        Returns a dict with projects and active configurations from a device
        name.

        :param domain: DB domain
        :param device_id: the device to return the information for.
        :return: a dict:
            {"project name": configuration uuid,
             ...}
        """
        configs = self.get_configurations_from_device_name(domain,
                                                           device_id)
        projects = dict()
        for config in configs:
            instance_id = config["instanceid"]
            for project in self.get_projects_from_device(domain,
                                                         instance_id):
                projects[project] = config["configid"]
        return projects

    async def get_projects_with_device(
            self, domain: str, device_id_part: str) -> list[dict[str, any]]:
        """
        Returns a list of dictionaries with data about projects that contain
        active configurations for a given device.

        :param domain: DB domain
        :param device_id_part: part of name of devices for which project data
                               must be returned.
        :return: a list of dicts:
            [{"projectname": name of project,
              "date": last modification timestamp for the project,
              "uuid": uuid of projecti
              "devices": list of ids of prj devices with the given part}, ...]
        """
        configs = await self.get_configurations_from_device_name_part(
            domain, device_id_part)
        projects = []
        for config in configs:
            device_uuid = config["device_uuid"]
            device_id = config["device_id"]
            for prj in await self.get_projects_data_from_device(domain,
                                                                device_uuid):
                prj_in_list = next((p for p in projects
                                    if p["uuid"] == prj["uuid"]), None)
                if prj_in_list:
                    # The project is already in the resulting list due to
                    # another device_id that matched the name part; add the
                    # device_id to the 'devices' attribute of the project.
                    prj_in_list["devices"].append(device_id)
                else:
                    prj["devices"] = [device_id]
                    projects.append(prj)

        return projects
