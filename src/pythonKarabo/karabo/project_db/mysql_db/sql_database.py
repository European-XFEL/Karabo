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
import weakref

from sqlmodel import SQLModel, select

from karabo.native import HashList

from ..bases import DatabaseBase, HandleABC
from ..util import ProjectDBError, make_str_if_needed, make_xml_if_needed
from .db_engine import init_db_engine, init_test_db_engine
from .db_reader import DbReader
from .db_writer import DbWriter
from .models import ProjectDomain
from .models_xml import (
    emit_device_config_xml, emit_device_instance_xml, emit_device_server_xml,
    emit_macro_xml, emit_project_xml, emit_scene_xml)


# TODO: HandleABC is an "artificial" dependency for the mysql case. Put here
#       to play along well with the isinstance check performed by DatabaseBase.
#       To be removed in a future cleanup.
class MySQLHandle(HandleABC):

    # As for the MySQL back-end domain related operations are not filesystem
    # operations, but database operations, a weak reference to the
    # ProjectDatabase must be provided to support domain operations.
    def __init__(self, db_weak_ref: weakref):
        super().__init__()
        self._db = db_weak_ref

    # region HandleABC compliance

    async def hasCollection(self, path):
        if self._db() is not None:
            path_parts = path.split('/')
            if len(path_parts) > 0:
                domain = path_parts[-1]
                return await self._db().domain_exists(domain)
        return False

    async def removeCollection(self, path):
        """"Abstract interface"""

    async def createCollection(self, path):
        if self._db() is not None:
            path_parts = path.split('/')
            if len(path_parts) > 0:
                domain = path_parts[-1]
                await self._db().add_domain(domain)

    def hasDocument(self, path):
        raise NotImplementedError

    def load(self, data, path):
        raise NotImplementedError

    def getDoc(self, name):
        raise NotImplementedError

    def getDocument(self, name):
        raise NotImplementedError

    # end region


class SQLDatabase(DatabaseBase):

    def __init__(self, user: str = "", password: str = "",
                 server: str = "", port: int = -1, db_name: str = "",
                 test_mode=False):
        super().__init__()
        if not test_mode:
            (self.db_engine, self.session_gen) = (
                init_db_engine(user, password, server, port, db_name))
        else:
            (self.db_engine, self.session_gen) = init_test_db_engine()
        self.reader = DbReader(self.session_gen)
        self.writer = DbWriter(self.session_gen)
        SQLModel.metadata.create_all(self.db_engine, checkfirst=True)

    def onEnter(self):
        return MySQLHandle(weakref.ref(self))

    async def list_domains(self) -> list[str]:
        domains = []
        with self.session_gen() as session:
            query = select(ProjectDomain)
            prj_domains = session.exec(query).all()
            for domain in prj_domains:
                domains.append(domain.name)
        return domains

    async def domain_exists(self, domain: str) -> bool:
        domains = await self.list_domains()
        return domain in domains

    async def add_domain(self, domain: str):
        if await self.domain_exists(domain):
            return
        with self.session_gen() as session:
            project_domain = ProjectDomain(name=domain)
            session.add(project_domain)
            session.commit()

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
            match item.strip().lower():
                case "project":
                    projects = self.reader.get_domain_projects(domain)
                    for project in projects:
                        result.append(project)
                case "macro":
                    macros = self.reader.get_domain_macros(domain)
                    for macro in macros:
                        result.append(macro)
                case "scene":
                    scenes = self.reader.get_domain_scenes(domain)
                    for scene in scenes:
                        result.append(scene)
                case "device_server":
                    servers = self.reader.get_domain_device_servers(domain)
                    for server in servers:
                        result.append(server)
                case "device_instance":
                    instances = self.reader.get_domain_device_instances(domain)
                    for instance in instances:
                        result.append(instance)
                case "device_config":
                    configs = self.reader.get_domain_device_configs(domain)
                    for config in configs:
                        result.append(config)
                case _:
                    raise ValueError(f'Unrecognized item_type, "{item}"')
        return result

    async def list_named_items(
            self, domain: str, item_type: str,
            simple_name: str) -> list[dict[str, any]]:
        """
        List items in domain which match item_type and simple_name

        :param domain: domain to list items from
        :param item_type: item_type to match
        :param simple_name: name to match
        :return: a list of dicts where each entry has keys: uuid, item_type
                 and simple_name
        """
        items = await self.list_items(domain, [item_type])
        named_items = []
        for item in items:
            if item['simple_name'] == simple_name:
                named_items.append(item)
        return named_items

    async def _load_project_item(self, uuid: str) -> dict[str, any] | None:
        item = None
        project = self.reader.get_project_from_uuid(uuid)
        if project:
            subprojects = self.reader.get_subprojects_of_project(project)
            scenes = self.reader.get_scenes_of_project(project)
            macros = self.reader.get_macros_of_project(project)
            servers = self.reader.get_device_servers_of_project(project)
            self.writer.register_project_load(project)
            item = {
                "uuid": uuid,
                "xml": emit_project_xml(
                    project, scenes, macros, servers, subprojects)}
        return item

    async def _load_scene_item(self, uuid: str) -> dict[str, any] | None:
        item = None
        scene = self.reader.get_scene_from_uuid(uuid)
        if scene:
            item = {
                "uuid": uuid,
                "xml": emit_scene_xml(scene)}
        return item

    async def _load_macro_item(self, uuid: str) -> dict[str, any] | None:
        item = None
        macro = self.reader.get_macro_from_uuid(uuid)
        if macro:
            item = {
                "uuid": uuid,
                "xml": emit_macro_xml(macro)}
        return item

    async def _load_server_item(self, uuid: str) -> dict[str, any] | None:
        item = None
        server = self.reader.get_device_server_from_uuid(uuid)
        if server:
            instances = self.reader.get_device_instances_of_server(server)
            item = {
                "uuid": uuid,
                "xml": emit_device_server_xml(server, instances)}
        return item

    async def _load_instance_item(self, uuid: str) -> dict[str, any] | None:
        item = None
        instance = self.reader.get_device_instance_from_uuid(uuid)
        if instance:
            configs = self.reader.get_device_configs_of_instance(instance)
            item = {
                "uuid": uuid,
                "xml": emit_device_instance_xml(instance, configs)}
        return item

    async def _load_config_item(self, uuid: str) -> dict[str, any] | None:
        item = None
        config = self.reader.get_device_config_from_uuid(uuid)
        if config:
            item = {
                "uuid": uuid,
                "xml": emit_device_config_xml(config)}
        return item

    async def _load_unknown_type(self, item_uuid):
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
        loaded_items = []
        for item_uuid in items_uuids:
            item = None
            match item_type:
                case "project":
                    item = await self._load_project_item(item_uuid)
                case "macro":
                    item = await self._load_macro_item(item_uuid)
                case "scene":
                    item = await self._load_scene_item(item_uuid)
                case "device_server":
                    item = await self._load_server_item(item_uuid)
                case "device_instance":
                    item = await self._load_instance_item(item_uuid)
                case "device_config":
                    item = await self._load_config_item(item_uuid)
                case _:
                    # item_type not known; try exhaustively by UUID
                    item = await self._load_unknown_type(item_uuid)
            if item is not None:
                loaded_items.append(item)
            else:
                raise ProjectDBError(
                    f'No item with UUID "{item_uuid}" of type "{item_type}" '
                    'found in the database. Load incomplete.')
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
                project = self.reader.get_project_from_uuid(uuid)
                if project:
                    current_modified = project.date
            case "macro":
                macro = self.reader.get_macro_from_uuid(uuid)
                if macro:
                    current_modified = macro.date
            case "scene":
                scene = self.reader.get_scene_from_uuid(uuid)
                if scene:
                    current_modified = scene.date
            case "device_server":
                server = self.reader.get_device_server_from_uuid(uuid)
                if server:
                    current_modified = server.date
            case "device_instance":
                instance = self.reader.get_device_instance_from_uuid(uuid)
                if instance:
                    current_modified = instance.date
            case "device_config":
                config = self.reader.get_device_config_from_uuid(uuid)
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
            # timestamp = strftime(DATE_FORMAT, gmtime())
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
            # timestamp = strftime(DATE_FORMAT, gmtime())
            item_tree.attrib['date'] = timestamp

        # XXX: Add a revision/alias to keep old code from blowing up
        item_tree.attrib['revision'] = '0'
        item_tree.attrib['alias'] = 'default'

        item_xml = make_str_if_needed(item_tree)

        match item_type:
            case "project":
                self.writer.save_project_item(domain, uuid, item_xml,
                                              timestamp)
            case "macro":
                self.writer.save_macro_item(uuid, item_xml, timestamp)
            case "scene":
                self.writer.save_scene_item(uuid, item_xml, timestamp)
            case "device_server":
                self.writer.save_device_server_item(uuid, item_xml, timestamp)
            case "device_instance":
                self.writer.save_device_instance_item(uuid, item_xml,
                                                      timestamp)
            case "device_config":
                self.writer.save_device_config_item(uuid, item_xml, timestamp)
            case _:
                raise ProjectDBError(
                    f"Saving of items of type '{item_type}' not supported")

        meta = {}
        meta['domain'] = domain
        meta['uuid'] = uuid
        meta['date'] = timestamp

        return meta

    async def get_configurations_from_device_name_part(
            self, domain: str, device_id_part: str,
            only_active: bool = False) -> list[dict[str, str]]:
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
        instances_by_name_part = (
            self.reader.get_domain_device_instances_by_name_part(
                domain, device_id_part))
        results = []
        for instance in instances_by_name_part:
            configs = self.reader.get_device_configs_of_instance(instance)
            for config in configs:
                if config.is_active or not only_active:
                    results.append(
                        {"config_id": config.uuid,
                         "device_uuid": instance.uuid,
                         "device_id": instance.name})
        return results

    async def get_configurations_from_device_name(
            self, domain: str, instance_id: str) -> list[dict[str, str]]:
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
        configs = []
        configs_name_part = self.get_configurations_from_device_name_part(
            domain, instance_id, only_active=True)
        for config in configs_name_part:
            configs.append({
                "configid": config['config_id'],
                "instanceid": config['device_uuid']
            })
        return configs

    async def get_projects_data_from_device(
            self, domain: str, uuid: str) -> list[dict[str, any]]:
        """
        Returns the project which contain a device instance with a given uuid

        :param domain: DB domain
        :param uuid: the uuid of the device instance from the database
        :return: a list containing project names, uuids and last modification
                 date.
        """
        instance = self.reader.get_device_instance_from_uuid(uuid)
        # From the DB structure, a given device instance only belongs to
        # one project
        project = self.reader.get_device_instance_project(instance)
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
                    model = self.reader.get_project_from_uuid(item_uuid)
                case "device_server":
                    model = self.reader.get_device_server_from_uuid(item_uuid)
                case "device_instance":
                    model = self.reader.get_device_instance_from_uuid(
                        item_uuid)
                case "device_config":
                    model = self.reader.get_device_config_from_uuid(item_uuid)
                case "macro":
                    model = self.reader.get_macro_from_uuid(item_uuid)
                case "scene":
                    model = self.reader.get_scene_from_uuid(item_uuid)
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
