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
import base64
import logging
from asyncio import gather
from collections.abc import Awaitable, Callable
from datetime import UTC
from pathlib import Path

from lxml import etree
from sqlalchemy.exc import IntegrityError
from sqlalchemy.orm import selectinload
from sqlmodel import SQLModel, select

from ..bases import DatabaseBase
from ..util import ProjectDBError, make_str_if_needed, make_xml_if_needed
from .db_engine import create_local_engine, create_remote_engine
from .models import (
    DeviceConfig, DeviceInstance, DeviceServer, Macro, Project, ProjectDomain,
    ProjectSubproject, Scene, SceneLinkedScene)
from .models_xml import (
    emit_config_xml, emit_device_xml, emit_macro_xml, emit_project_xml,
    emit_scene_xml, emit_server_xml)
from .utils import (
    date_utc_to_local, datetime_from_str, datetime_now, datetime_str_now,
    datetime_to_str, get_scene_links, get_trashed)

logger = logging.getLogger(__file__)


class SQLDatabase(DatabaseBase):

    def __init__(self, user: str = "", password: str = "",
                 server: str = "", port: int = -1, db_name: str = "local.db",
                 local: bool = False):
        super().__init__()
        if local:
            self.db_engine, self.session_gen = create_local_engine(db_name)
        else:
            self.db_engine, self.session_gen = (
                create_remote_engine(user, password, server, port, db_name))
        self.dbName = db_name
        self.initialized = False
        self.local = local

    async def initialize(self):
        async with self.db_engine.begin() as conn:
            await conn.run_sync(SQLModel.metadata.create_all)

        self.initialized = True

    async def assure_domains(self, required_domains: list[str]):
        # There's a list of initial domains defined and there's no
        # domain stored in the DB. Initialize the domains.
        for domain in required_domains:
            await self.add_domain(domain)

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

    async def domain_exists(self, domain: str) -> bool:
        domains = await self.list_domains()
        return domain in domains

    async def list_items(
            self, domain: str, item_types: list[str] | None = None
    ) -> list[dict[str, any]]:
        """
        List items in domain which match item_types if given, or all items
        if not given.
        :param domain: domain to list items from
        :param item_types: list or tuple of item_types to list
        :return: list of dicts with keys: uuid, item_type, simple_name
        """
        if not await self.domain_exists(domain):
            raise ProjectDBError(f'Domain "{domain}" not found')

        if item_types is None:
            item_types = [
                "project", "macro", "scene",
                "device_server", "device_instance", "device_config"
            ]

        reader_map: dict[str, Callable[[str], Awaitable[list]]] = {
            "project": self._get_domain_projects,
            "macro": self._get_domain_macros,
            "scene": self._get_domain_scenes,
            "device_server": self._get_domain_device_servers,
            "device_instance": self._get_domain_device_instances,
            "device_config": self._get_domain_device_configs,
        }
        tasks = [reader_map[item_type](domain) for item_type in item_types]
        results = await gather(*tasks)

        # Flatten results into a single list
        result = [item for group in results for item in group]
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

    async def list_domains(self):
        return [d.name for d in await self._execute_all(select(ProjectDomain))]

    async def get_devices_from_domain(
            self, domain: str) -> list[dict[str, any]]:
        """Return an information about the device topology """
        query = (
            select(DeviceInstance, DeviceServer, Project)
            .join(DeviceInstance.device_server)
            .join(DeviceServer.project)
            .join(Project.project_domain)
            .where(ProjectDomain.name == domain)
            .where(Project.is_trashed.is_(False))
        )
        async with self.session_gen() as session:
            result = await session.exec(query)
            rows = result.all()
            instances = []
            for device_instance, device_server, project in rows:
                instances.append({
                    "device_uuid": device_instance.uuid,
                    "device_name": device_instance.name,
                    "device_class": device_instance.class_id,
                    "server_name": device_server.name,
                    "project_uuid": project.uuid,
                    "project_name": project.name,
                })

            return instances

    async def _emit_project_item(self, uuid: str) -> dict[str, any] | None:
        """Internal emitter method to load a project item"""
        item = None
        project = await self._get_project_from_uuid(uuid)
        if project:
            subprojects, scenes, macros, servers = await gather(
                self._get_subprojects_of_project(project),
                self._get_scenes_of_project(project),
                self._get_macros_of_project(project),
                self._get_servers_of_project(project)
            )
            await self._register_project_load(project)
            item = {
                "uuid": uuid,
                "xml": emit_project_xml(
                    project, scenes, macros, servers, subprojects)}
        return item

    async def _emit_scene_item(self, uuid: str) -> dict[str, any] | None:
        item = None
        scene = await self._get_scene_from_uuid(uuid)
        if scene:
            item = {
                "uuid": uuid,
                "xml": emit_scene_xml(scene)}
        return item

    async def _emit_macro_item(self, uuid: str) -> dict[str, any] | None:
        macro = await self._get_macro_from_uuid(uuid)
        if not macro:
            return
        item = {"uuid": uuid,
                "xml": emit_macro_xml(macro)}
        return item

    async def _emit_server_item(self, uuid: str) -> dict[str, any] | None:
        server = await self._get_server_from_uuid(uuid)
        if not server:
            return
        instances = await self._get_devices_of_server(server)
        item = {"uuid": uuid,
                "xml": emit_server_xml(server, instances)}
        return item

    async def _emit_device_item(self, uuid: str) -> dict[str, any] | None:
        instance = await self._get_device_from_uuid(uuid)
        if not instance:
            return
        configs = await self._get_configs_of_device(instance)
        item = {"uuid": uuid,
                "xml": emit_device_xml(instance, configs)}
        return item

    async def _emit_config_item(self, uuid: str) -> dict[str, any] | None:
        config = await self._get_config_from_uuid(uuid)
        if not config:
            return
        item = {"uuid": uuid,
                "xml": emit_config_xml(config)}
        return item

    async def _emit_unknown(self, item_uuid: str):
        item = await self._emit_project_item(item_uuid)
        if item is None:
            item = await self._emit_scene_item(item_uuid)
        if item is None:
            item = await self._emit_macro_item(item_uuid)
        if item is None:
            item = await self._emit_server_item(item_uuid)
        if item is None:
            item = await self._emit_device_item(item_uuid)
        if item is None:
            item = await self._emit_config_item(item_uuid)
        if item is None:
            raise ProjectDBError(
                f"Item with uuid {item_uuid} not found.")
        return item

    async def load_item(
            self, domain: str, items_uuids: list[str],
            item_type: str | None = None) -> list[dict[str, any]]:
        """
        Loads a list of items of a specified type from a given domain.

        :param domain: domain name (not used for SQL; legacy from ExistDB)
        :param items_uuids: list of UUIDs to load
        :param item_type: optional type of the items to be loaded
        :return: List of dicts with keys "uuid" and "xml"
        :raises: ProjectDBError if any item can't be found
        """
        reader_map: dict[str, Callable[[str], Awaitable[dict[str, any]]]] = {
            "project": self._emit_project_item,
            "macro": self._emit_macro_item,
            "scene": self._emit_scene_item,
            "device_server": self._emit_server_item,
            "device_instance": self._emit_device_item,
            "device_config": self._emit_config_item,
        }
        if item_type is not None:
            loader_func = reader_map.get(item_type, None)
            if loader_func is None:
                raise ProjectDBError(f'Unrecognized item_type "{item_type}"')
            futures = [loader_func(uuid) for uuid in items_uuids]
        else:
            # Try to resolve each UUID using exhaustive strategy
            futures = [self._emit_unknown(uuid) for uuid in items_uuids]

        loaded_items = await gather(*futures)
        return loaded_items

    async def _check_for_modification(self, uuid: str, client_date: str,
                                      item_type: str) -> tuple[bool, str]:
        """ Check whether the item with of the given `domain` and `uuid` was
        modified

        :param uuid: the item's uuid
        :param client_date: the item's last modified time stamp
        :param item_type: the type of the item to be checked (e.g. 'project')
        :return: A tuple stating whether the item was modified inbetween and a
                 string describing the reason
        """
        last_modified = None
        match item_type:
            case "project":
                project = await self._get_project_from_uuid(uuid)
                if project:
                    last_modified = project.date
            case "macro":
                macro = await self._get_macro_from_uuid(uuid)
                if macro:
                    last_modified = macro.date
            case "scene":
                scene = await self._get_scene_from_uuid(uuid)
                if scene:
                    last_modified = scene.date
            case "device_server":
                server = await self._get_server_from_uuid(uuid)
                if server:
                    last_modified = server.date
            case "device_instance":
                instance = await self._get_device_from_uuid(
                    uuid)
                if instance:
                    last_modified = instance.date
            case "device_config":
                config = await self._get_config_from_uuid(uuid)
                if config:
                    last_modified = config.date

        if last_modified:
            client_modified = datetime_from_str(client_date)
            # `last_modified`` was read from the DB as a naive datetime, but we
            # know it is in UTC. Add the timezone information so it can be
            # compared to the non-naive obtained from client_date.
            last_modified = last_modified.replace(tzinfo=UTC)
            if client_modified < last_modified:
                # The data has been changed in the database between the
                # client_date that the GUI client sent and the current time.
                # Need to convert UTC timestamps to local timestamps
                lm = date_utc_to_local(last_modified)
                cm = date_utc_to_local(client_modified)
                text = (
                    f'Item of type "{item_type}" with uuid "{uuid}" '
                    f'has been modified in the DB ({lm}) '
                    f'after its last retrieval by the client ({cm}).')
                return True, text

        return False, ""

    async def save_item(self, domain: str, uuid: str, item_xml: str,
                        overwrite: bool = False) -> str:
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

        :return: date (str) of saving


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
        if not item_tree.attrib.get('date'):
            timestamp = datetime_str_now()
            item_tree.attrib['date'] = timestamp
        else:
            modified, reason = await self._check_for_modification(
                uuid, item_tree.attrib['date'], item_type)
            if modified:
                simple_name = item_tree.attrib.get('simple_name', '')
                message = (
                    f"The <b>{item_type}</b> item <b>{simple_name}</b> "
                    f"could not be saved: {reason}")
                raise ProjectDBError(message)
            # Update time stamp
            timestamp = datetime_str_now()
            item_tree.attrib['date'] = timestamp

        # XXX: Add a revision/alias to keep old code from blowing up
        # Is this needed?
        item_tree.attrib['revision'] = '0'
        item_tree.attrib['alias'] = 'default'

        # And back conversion to string
        item_xml = make_str_if_needed(item_tree)

        match item_type:
            case "project":
                await self._save_project_item(
                    domain, uuid, item_xml, timestamp)
            case "macro":
                await self._save_macro_item(
                    uuid, item_xml, timestamp)
            case "scene":
                await self._save_scene_item(
                    uuid, item_xml, timestamp)
            case "device_server":
                await self._save_device_server_item(
                    uuid, item_xml, timestamp)
            case "device_instance":
                await self._save_device_instance_item(
                    uuid, item_xml, timestamp)
            case "device_config":
                await self._save_device_config_item(
                    uuid, item_xml, timestamp)
            case _:
                raise ProjectDBError(
                    f"Saving of items of type '{item_type}' not supported")

        return timestamp

    async def _get_configuration_from_device_part(
            self, domain: str, name_part: str,
            only_active: bool = False
    ) -> list[dict[str, str]]:
        """
        Returns a list of configurations for a given device

        :param domain: DB domain
        :param name_part: part of device name; search is case-insensitive.
        :param only_active: if True only returns the active configurations

        :return: a list of dicts:

            [{"config_id": uuid of the configuration,
              "device_uuid": device instance uuid in the DB,
              "device_id": device instance id},
              ...
            ]
        """
        instances = await self._get_domain_devices_by_name_part(
            domain, name_part
        )
        results = []
        for instance in instances:
            configs = await self._get_configs_of_device(instance)
            filtered = [{"config_id": config.uuid,
                         "device_uuid": instance.uuid,
                         "device_id": instance.name}
                        for config in configs
                        if config.is_active or not only_active]
            results.extend(filtered)

        return results

    async def _get_configurations_from_device_name(
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
        configs = await self._get_configuration_from_device_part(
            domain, instance_id, only_active=True)

        items = [{"configid": config["config_id"],
                  "instanceid": config["device_uuid"]}
                 for config in configs]
        return items

    async def _get_project_from_device_uuid(
            self, domain: str, uuid: str) -> Project:
        """
        Returns the project which contain a device instance with a given uuid

        :param domain: DB domain
        :param uuid: the uuid of the device instance from the database
        :return: a list containing project names, uuids and last modification
                 date.
        """
        instance = await self._get_device_from_uuid(uuid)
        # From the DB structure, a given device instance only belongs to
        # one project
        projects = await self._execute_first(
            select(Project).join(DeviceServer).where(
                DeviceServer.id == instance.device_server_id))
        return projects

    async def update_trashed(self, **info) -> None:
        """ Update trashed attribute for project

        :raises: ProjectDBError on failure
        """
        item_uuid = info['uuid']
        item_type = info['item_type']
        assert item_type == "project", "Only projects can be trashed"
        value = info["value"]
        assert isinstance(value, bool)

        model = await self._get_project_from_uuid(
            item_uuid)
        if model is None:
            raise ProjectDBError(
                f"No item of type '{item_type}' with UUID '{item_uuid}' "
                "found in the database")

        model.is_trashed = value
        async with self.session_gen() as session:
            session.add(model)
            await session.commit()

    async def get_device_config_from_device_uuid(self, device_uuid: str):
        device_instance = await self._execute_first(
            select(DeviceInstance)
            .options(selectinload(DeviceInstance.configs))
            .where(DeviceInstance.uuid == device_uuid))
        if not device_instance:
            return None

        # Find the first config with name "default"
        configuration = next(
            (config.config_data for config in device_instance.configs
             if config.name == "default"), None)

        return configuration

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
        configs = await self._get_configurations_from_device_name(
            domain, device_id)
        projects = dict()
        for config in configs:
            instance_id = config["instanceid"]
            project = await self._get_project_from_device_uuid(
                domain, instance_id)
            projects[project.name] = config["configid"]
        return projects

    async def get_projects_with_device(
            self, domain: str, name_part: str) -> list[dict[str, any]]:
        """
        Returns a list of dictionaries with data about projects from a given
        domain that contain active configurations for a given device.

        :param domain: domain of the projects to be returned
        :param name_part: part of name of devices for which project data
                               must be returned.
        :return: a list of dicts:
            [{"project_name": name of project,
              "date": last modification timestamp for the project,
              "uuid": uuid of project,
              "devices": list of ids of prj devices with the given part},
            ...]
        """
        items = await self._execute_all(
            select(Project, DeviceInstance)
            .join(ProjectDomain).join(DeviceServer).join(DeviceInstance)
            .where(ProjectDomain.name == domain)
            .where(Project.is_trashed.is_(False))
            .filter(DeviceInstance.name.ilike(f'%{name_part}%'))
        )

        projects = {}
        for item in items:
            if item.Project.id not in projects.keys():
                projects[item.Project.id] = {
                    "project_name": item.Project.name,
                    "date": datetime_to_str(item.Project.date),
                    "uuid": item.Project.uuid,
                    "items": []}
            projects[item.Project.id]["items"].append(
                item.DeviceInstance.name)
        return list(projects.values())

    async def get_projects_with_server(
            self, domain: str, name_part: str) -> list[dict[str, any]]:
        """
        Returns a list of dictionaries with data about projects that
        contain macros.

        :param domain: domain of the projects to be returned.
        :param name_part: part of name of macro for which project data
                          must be returned.
        :return: a list of dicts:
            [
            {"project_name": name of project,
             "date": last modification timestamp for the project,
             "uuid": uuid of project
             "items": list of ids of prj servers with the given part},
             ...]
        """
        items = await self._execute_all(
            select(Project, DeviceServer)
            .join(ProjectDomain).join(DeviceServer)
            .where(ProjectDomain.name == domain)
            .where(Project.is_trashed.is_(False))
            .filter(DeviceServer.name.ilike(f'%{name_part}%')))

        projects = {}
        for item in items:
            if item.Project.id not in projects.keys():
                projects[item.Project.id] = {
                    "project_name": item.Project.name,
                    "date": datetime_to_str(item.Project.date),
                    "uuid": item.Project.uuid,
                    "items": []}
            projects[item.Project.id]["items"].append(
                item.DeviceServer.name)
        return list(projects.values())

    async def get_projects_with_macro(
            self, domain: str, name_part: str) -> list[dict[str, any]]:
        """
        Returns a list of dictionaries with data about projects that
        contain macros.

        :param domain: domain of the projects to be returned
        :param name_part: part of name of macro for which project data
                          must be returned.
        :return: a list of dicts:
            [
            {"project_name": name of project,
             "date": last modification timestamp for the project,
             "uuid": uuid of project
             "macros": list of ids of prj devices with the given part},
             ...]
        """
        items = await self._execute_all(
            select(Project, Macro)
            .join(ProjectDomain).join(Macro)
            .where(ProjectDomain.name == domain)
            .where(Project.is_trashed.is_(False))
            .filter(Macro.name.ilike(f'%{name_part}%')))

        projects = {}
        for item in items:
            if item.Project.id not in projects.keys():
                projects[item.Project.id] = {
                    "project_name": item.Project.name,
                    "date": datetime_to_str(item.Project.date),
                    "uuid": item.Project.uuid,
                    "items": []}
            projects[item.Project.id]["items"].append(
                item.Macro.name)
        return list(projects.values())

    # region Reader

    async def _execute_all(self, base_query, order_by=None):
        if order_by is not None:
            base_query = base_query.order_by(order_by)
        async with self.session_gen() as session:
            result = await session.exec(base_query)
            return result.all()

    async def _execute_first(self, base_query, order_by=None):
        if order_by is not None:
            base_query = base_query.order_by(order_by)
        async with self.session_gen() as session:
            result = await session.exec(base_query)
            return result.first()

    # endregion
    # region Domain

    async def _get_domain_projects(self, domain: str):
        result = await self._execute_all(
            select(Project).join(ProjectDomain).where(
                ProjectDomain.name == domain))
        return [{
            "uuid": p.uuid,
            "item_type": "project",
            "simple_name": p.name,
            "is_trashed": p.is_trashed,
            "date": datetime_to_str(p.date),
        } for p in result]

    async def _get_domain_macros(self, domain):
        items = await self._execute_all(
            select(Macro)
            .join(Project).join(ProjectDomain)
            .where(ProjectDomain.name == domain))
        return [{"uuid": i.uuid, "item_type": "macro",
                 "simple_name": i.name, "date": datetime_to_str(i.date)}
                for i in items]

    async def _get_domain_scenes(self, domain: str):
        items = await self._execute_all(
            select(Scene)
            .join(Project).join(ProjectDomain)
            .where(ProjectDomain.name == domain))
        return [{"uuid": i.uuid, "item_type": "scene",
                 "simple_name": i.name, "date": datetime_to_str(i.date)}
                for i in items]

    async def _get_domain_device_servers(self, domain: str):
        items = await self._execute_all(
            select(DeviceServer)
            .join(Project).join(ProjectDomain)
            .where(ProjectDomain.name == domain)
        )
        return [{"uuid": i.uuid, "item_type": "device_server",
                 "simple_name": i.name, "date": datetime_to_str(i.date)}
                for i in items]

    async def _get_domain_device_instances(self, domain):
        items = await self._execute_all(
            select(DeviceInstance)
            .join(DeviceServer).join(Project).join(ProjectDomain)
            .where(ProjectDomain.name == domain))
        return [{"uuid": i.uuid, "item_type": "device_instance",
                 "simple_name": i.name, "date": datetime_to_str(i.date)}
                for i in items]

    async def _get_domain_device_configs(self, domain: str):
        items = await self._execute_all(
            select(DeviceConfig)
            .join(DeviceInstance).join(DeviceServer).join(Project)
            .join(ProjectDomain)
            .where(ProjectDomain.name == domain))
        return [{"uuid": i.uuid, "item_type": "device_config",
                 "simple_name": i.name, "date": datetime_to_str(i.date)}
                for i in items]

    # endregion

    async def _get_subprojects_of_project(self, project: Project):
        links = await self._execute_all(
            select(ProjectSubproject).where(
                ProjectSubproject.project_id == project.id),
            order_by=ProjectSubproject.order)
        subprojects = []
        futures = [self._execute_first(select(Project).where(
            Project.id == link.subproject_id)) for link in links]
        if futures:
            subprojects = await gather(*futures)
            subprojects = [sub for sub in subprojects if sub]
        return subprojects

    async def _get_scenes_of_project(self, project):
        return await self._execute_all(
            select(Scene).where(Scene.project_id == project.id),
            order_by=Scene.order)

    async def _get_macros_of_project(self, project):
        return await self._execute_all(
            select(Macro).where(Macro.project_id == project.id),
            order_by=Macro.order)

    async def _get_servers_of_project(self, project):
        return await self._execute_all(
            select(DeviceServer).where(DeviceServer.project_id == project.id),
            order_by=DeviceServer.order)

    async def _get_devices_of_server(self, server):
        return await self._execute_all(
            select(DeviceInstance).where(
                DeviceInstance.device_server_id == server.id),
            order_by=DeviceInstance.order)

    async def _get_configs_of_device(self, instance):
        return await self._execute_all(
            select(DeviceConfig).where(
                DeviceConfig.device_instance_id == instance.id),
            order_by=DeviceConfig.order)

    async def _get_project_from_id(self, id: int):
        project = await self._execute_first(
            select(Project).where(Project.id == id))
        return project

    async def _get_server_from_id(self, id: int):
        server = await self._execute_first(
            select(DeviceServer).where(DeviceServer.id == id))
        return server

    # region UUID

    async def _get_project_from_uuid(self, uuid):
        project = await self._execute_first(
            select(Project).where(Project.uuid == uuid))
        return project

    async def _get_scene_from_uuid(self, uuid):
        return await self._execute_first(
            select(Scene).where(Scene.uuid == uuid))

    async def _get_macro_from_uuid(self, uuid):
        return await self._execute_first(
            select(Macro).where(Macro.uuid == uuid))

    async def _get_server_from_uuid(self, uuid):
        return await self._execute_first(
            select(DeviceServer).where(DeviceServer.uuid == uuid))

    async def _get_device_from_uuid(self, uuid):
        return await self._execute_first(
            select(DeviceInstance).where(DeviceInstance.uuid == uuid))

    async def _get_config_from_uuid(self, uuid):
        return await self._execute_first(
            select(DeviceConfig).where(DeviceConfig.uuid == uuid))

    # endregion
    # region namepart

    async def _get_domain_devices_by_name_part(
            self, domain: str, name_part: str):
        return await self._execute_all(
            select(DeviceInstance)
            .join(DeviceServer).join(Project).join(ProjectDomain)
            .where(ProjectDomain.name == domain)
            .filter(DeviceInstance.name.ilike(f'%{name_part}%')))

    # end region
    # region Writer

    async def _register_project_load(self, project: Project):
        async with self.session_gen() as session:
            project.last_loaded = datetime_now()
            session.add(project)
            await session.commit()

    async def _save_project_item(
            self, domain: str, uuid: str, xml: str, timestamp: str):
        project_xml = etree.fromstring(xml)
        date = datetime_from_str(timestamp)

        # Save the project attributes
        project_id = None  # the id of the updated or new project
        async with self.session_gen() as session:
            result = await session.exec(
                select(ProjectDomain).where(ProjectDomain.name == domain))
            project_domain = result.first()
            # domain is expected to correspond to a valid domain.
            assert project_domain is not None

            result = await session.exec(
                select(Project).where(Project.uuid == uuid))
            project = result.first()

            attrs = project_xml.attrib
            if not project:
                # There's still no record for the project in the DB
                trashed = get_trashed(attrs.get('is_trashed'))
                project = Project(
                    uuid=uuid,
                    name=attrs['simple_name'],
                    is_trashed=trashed,
                    date=date,
                    project_domain_id=project_domain.id)
                session.add(project)
                # The commit / refresh sequence is needed for the case of a
                # new project. A new project only gets its ID after being
                # stored in the database.
                await session.commit()
                await session.refresh(project)
            else:
                # There's a record for the project in the DB - update its
                # attributes from the data in the xml
                project.name = attrs['simple_name']
                project.is_trashed = get_trashed(attrs['is_trashed'])
                project.date = date
                project.project_domain_id = project_domain.id
                session.add(project)

            # Save the children references to the project and
            # their relative order. For each child type, the items of that
            # type currently linked to the project must be first unlinked
            # to the project.
            project_elements = (
                project_xml.getchildren()[0].getchildren()[0]
                if project_xml.getchildren()
                and project_xml.getchildren()[0].getchildren() else None)

            group_map = {}
            if project_elements is not None:
                group_map = {child.tag.lower(): child
                             for child in project_elements.getchildren()}

            # several child elements
            macro_group = group_map.get("macros", None)
            scene_group = group_map.get("scenes", None)
            servers_group = group_map.get("servers", None)
            subprojects_group = group_map.get("subprojects", None)

            project_id = project.id

            # --- Macros ---
            result = await session.exec(
                select(Macro).where(Macro.project_id == project_id))
            existing = result.all()
            for macro in existing:
                macro.project_id = None
                macro.order = 0
                session.add(macro)

            macro_index = 0
            if macro_group is not None:
                for macro_elem in macro_group.getchildren():
                    macro_uuid = macro_elem.getchildren()[0].text
                    result = await session.exec(
                        select(Macro).where(Macro.uuid == macro_uuid))
                    macro = result.first()
                    if not macro:
                        raise ProjectDBError(
                            f'Macro with uuid "{macro_uuid}" not found in the '
                            'database. Cannot link the macro to project '
                            f'"{project.name}" ({project.uuid})')
                    macro.project_id = project_id
                    macro.order = macro_index
                    session.add(macro)
                    macro_index += 1

            # --- Scenes ---
            result = await session.exec(
                select(Scene).where(Scene.project_id == project_id))
            existing = result.all()
            for scene in existing:
                scene.project_id = None
                scene.order = 0
                session.add(scene)

            if scene_group is not None:
                scene_index = 0
                for scene_element in scene_group.getchildren():
                    scene_uuid = (
                        scene_element.getchildren()[0].text
                        if len(scene_element.getchildren()) > 0 else None)
                    # The legacy unit tests had the scene UUID as an attribute
                    # of a <xml> tag
                    if (scene_uuid is None
                            and "uuid" in scene_element.attrib.keys()):
                        scene_uuid = scene_element.attrib['uuid']

                    result = await session.exec(
                        select(Scene).where(Scene.uuid == scene_uuid))
                    scene = result.first()
                    if not scene:
                        raise ProjectDBError(
                            f'Scene with uuid "{scene_uuid}" not found in the '
                            'database. Cannot link the scene to project '
                            f'"{project.name}" ({project.uuid})')
                    scene.project_id = project_id
                    scene.order = scene_index
                    session.add(scene)
                    scene_index += 1

            # --- Servers ---

            result = await session.exec(
                select(DeviceServer).where(
                    DeviceServer.project_id == project_id))
            existing = result.all()
            # erase order and reference
            for server in existing:
                server.project_id = None
                server.order = 0
                session.add(server)

            if servers_group is not None:
                server_index = 0
                for server_element in servers_group.getchildren():
                    server_uuid = server_element.getchildren()[0].text
                    result = await session.exec(
                        select(DeviceServer).where(
                            DeviceServer.uuid == server_uuid))
                    server = result.first()
                    if not server:
                        raise ProjectDBError(
                            f'Server with uuid "{server_uuid}" not found in '
                            'the database. Cannot link the server to project'
                            f'"{project.name}" ({project.uuid})')
                        continue
                    server.project_id = project_id
                    server.order = server_index
                    session.add(server)
                    server_index += 1

            # --- Subprojects ---

            # Initially all the previously saved subproject are to be
            # potentially removed
            # Note: subprojects are only a reference to projects, not real
            #       entities stored in the database like scenes and macros.
            result = await session.exec(
                select(ProjectSubproject).where(
                    ProjectSubproject.project_id == project_id))
            existing = result.all()
            subprojects_to_delete = {
                subproject.subproject_id for subproject in existing}

            # Iterate over the subprojects to be saved
            if subprojects_group is not None:
                subproject_index = 0
                for sub_element in subprojects_group.getchildren():
                    subproject_uuid = sub_element.getchildren()[0].text
                    result = await session.exec(
                        select(Project).where(
                            Project.uuid == subproject_uuid))
                    subproject = result.first()
                    if not subproject:
                        raise ProjectDBError(
                            f'Subproject with uuid "{server_uuid}" not found '
                            'in the database. Cannot link the subproject to '
                            f'project "{project.name}" ({project.uuid})')
                    # Search for the subproject among the current projects, and
                    # either add it, update it or leave it in the list of
                    # subproject to be removed
                    if subproject.id in subprojects_to_delete:
                        # The project was in the DB and should be kept - just
                        # update it
                        subprojects_to_delete.remove(subproject.id)
                        query = select(ProjectSubproject).where(
                            ProjectSubproject.project_id == project_id,
                            ProjectSubproject.subproject_id == subproject.id)
                        result = await session.exec(query)
                        subproject_update = result.first()
                        if subproject_update:
                            subproject_update.order = subproject_index
                        session.add(subproject_update)
                    else:
                        # The project wasn't in the DB - add it
                        subproject_new = ProjectSubproject(
                            project_id=project_id,
                            subproject_id=subproject.id,
                            order=subproject_index)
                        session.add(subproject_new)
                    subproject_index += 1

            # Remove the subprojects that were in the DB before but shouldn't
            # be there after the save
            for subproject_id in subprojects_to_delete:
                result = await session.exec(
                    select(ProjectSubproject).where(
                        ProjectSubproject.project_id == project_id,
                        ProjectSubproject.subproject_id == subproject_id))
                to_delete = result.first()
                if to_delete:
                    await session.delete(to_delete)

            await session.commit()

    async def _save_macro_item(self, uuid: str, xml: str, timestamp: str):
        macro_obj = etree.fromstring(xml)
        macro_uuid = macro_obj.attrib["uuid"]
        macro_name = macro_obj.attrib["simple_name"]
        # In MySQL the macro bodies are not Base64 encoded
        macro_body = base64.b64decode(macro_obj.getchildren()[0].text)

        date = datetime_from_str(timestamp)

        async with self.session_gen() as session:
            result = await session.exec(
                select(Macro).where(Macro.uuid == uuid))
            macro = result.first()
            if macro:
                # A macro is being updated
                macro.name = macro_name
                macro.date = date
                macro.body = macro_body
            else:
                # The macro is new
                macro = Macro(
                    uuid=macro_uuid,
                    name=macro_name,
                    date=date,
                    body=macro_body)
            session.add(macro)
            await session.commit()

    async def _save_scene_item(self, uuid: str, xml: str, timestamp: str):
        date = datetime_from_str(timestamp)
        scene_obj = etree.fromstring(xml)
        scene_uuid = scene_obj.attrib["uuid"]
        scene_name = scene_obj.attrib["simple_name"]
        scene_svg_data = (
            etree.tostring(scene_obj.getchildren()[0]).decode("UTF-8")
            if len(scene_obj.getchildren()) > 0 else "")

        async with self.session_gen() as session:
            result = await session.exec(
                select(Scene).where(Scene.uuid == uuid))
            scene = result.first()
            if scene:
                # A scene is being updated

                # Remove all the scene links originated from the scene
                query_links = (
                    select(SceneLinkedScene)
                    .where(SceneLinkedScene.scene_id == scene.id))
                result_links = await session.exec(query_links)
                links = result_links.all()
                for link in links:
                    await session.delete(link)

                scene.name = scene_name
                scene.date = date
                scene.svg_data = scene_svg_data
            else:
                # The scene is new
                scene = Scene(
                    uuid=scene_uuid,
                    name=scene_name,
                    date=date,
                    svg_data=scene_svg_data)
            session.add(scene)
            await session.commit()
            if scene.id is None:
                await session.refresh(scene)
            await self._save_scene_links(scene)

    async def _save_scene_links(self, scene: Scene):
        target_uuids = get_scene_links(scene)
        for uuid in target_uuids:
            target_scene = await self._get_scene_from_uuid(uuid)
            if target_scene is None:
                # XXX: Here we are graceful
                logger.error(
                    f"Target scene with UUID '{uuid}' doesn't exist in "
                    f"the database. Skipping scene link registration.")
                continue
            async with self.session_gen() as session:
                try:
                    linked_scene = SceneLinkedScene(
                        scene_id=scene.id,
                        linked_scene_id=target_scene.id)
                    session.add(linked_scene)
                    await session.commit()
                except IntegrityError:
                    # A scene can have multiple scene links to the same scene.
                    # An attempt to insert the second repeated link will
                    # violate the unique constraint for the combination of
                    # source and target scene of the link. This is not an error
                    # and can be ignored. The unique constraint should be kept
                    # though as it prevents storage of repeated information.
                    await session.rollback()

    async def _save_device_config_item(
            self, uuid: str, xml: str, timestamp: str):
        date = datetime_from_str(timestamp)
        config_obj = etree.fromstring(xml)
        config_uuid = config_obj.attrib["uuid"]
        config_name = config_obj.attrib["simple_name"]
        config_data = (
            etree.tostring(config_obj.getchildren()[0]).decode("UTF-8")
            if len(config_obj.getchildren()) > 0 else "")

        async with self.session_gen() as session:
            result = await session.exec(
                select(DeviceConfig).where(DeviceConfig.uuid == uuid))
            config = result.first()
            if config:
                # A device config is being updated
                config.name = config_name
                config.config_data = config_data
                config.date = date
            else:
                # A new device config
                config = DeviceConfig(
                    uuid=config_uuid,
                    name=config_name,
                    config_data=config_data,
                    date=date)

            session.add(config)
            await session.commit()

    async def _save_device_instance_item(
            self, uuid: str, xml: str, timestamp: str):
        # XXX: Why is the uuid not taken here?
        date = datetime_from_str(timestamp)
        instance_element = etree.fromstring(xml)

        uuid = instance_element.attrib["uuid"]
        instance_tag = instance_element.getchildren()[0]
        instance_id = instance_tag.attrib['instance_id']
        class_id = instance_tag.attrib['class_id']
        active_uuid = instance_tag.attrib['active_uuid']
        config_objs = instance_tag.getchildren()

        # Insert or update the device instance in the DB
        async with self.session_gen() as session:
            result = await session.exec(
                select(DeviceInstance).where(DeviceInstance.uuid == uuid))
            instance = result.first()
            if instance:
                # Updates the device instance
                instance.name = instance_id
                instance.class_id = class_id
                instance.date = date
                session.add(instance)
            else:
                # The device instance must be added to the DB
                instance = DeviceInstance(
                    uuid=uuid,
                    name=instance_id,
                    class_id=class_id,
                    date=date)
                session.add(instance)
                await session.commit()
                await session.refresh(instance)

            # Saves the device configs linked to the device instance that
            # has been just saved. First the currently linked configs
            # must be unlinked
            result = await session.exec(
                select(DeviceConfig).where(
                    DeviceConfig.device_instance_id == instance.id))
            existing = result.all()
            for config in existing:
                config.device_instance_id = None
                config.order = 0
                session.add(config)

            config_index = 0
            for config_obj in config_objs:
                config_obj_uuid = config_obj.attrib["uuid"]
                result = await session.exec(
                    select(DeviceConfig).where(
                        DeviceConfig.uuid == config_obj_uuid))
                config = result.first()
                if not config:
                    raise ProjectDBError(
                        f'Device config with uuid "{config_obj_uuid}" not '
                        "found in the database. Cannot link the config to "
                        f'instance "{instance.name}" ({instance.uuid})')
                config.device_instance_id = instance.id
                config.order = config_index
                config.is_active = (active_uuid == config_obj_uuid)
                config_index += 1
                session.add(config)

            await session.commit()

    async def _save_device_server_item(
            self, uuid: str, xml: str, timestamp: str):
        date = datetime_from_str(timestamp)
        server_obj = etree.fromstring(xml)
        server_uuid = server_obj.attrib["uuid"]
        server_name = server_obj.attrib["simple_name"]
        server_tag = (server_obj.getchildren()[0]
                      if len(server_obj.getchildren()) > 0 else None)
        instance_elements = (server_tag.getchildren() if server_tag is not None
                             else [])

        async with self.session_gen() as session:
            result = await session.exec(
                select(DeviceServer).where(DeviceServer.uuid == uuid))
            server = result.first()
            if server:
                # An existing device server is being saved
                server.name = server_name
                server.date = date
                session.add(server)
            else:
                # A new device server is being saved
                server = DeviceServer(
                    uuid=server_uuid,
                    name=server_name,
                    date=date)
                session.add(server)
                await session.commit()
                await session.refresh(server)

            # Saves the device instances linked to the device server just saved
            # First unlinks all the currently linked device instances.
            result = await session.exec(
                select(DeviceInstance).where(
                    DeviceInstance.device_server_id == server.id))
            existing = result.all()
            for instance in existing:
                instance.device_server_id = None
                instance.order = 0
                session.add(instance)

            instance_index = 0
            for instance_element in instance_elements:
                instance_element_uuid = instance_element.attrib["uuid"]
                result = await session.exec(
                    select(DeviceInstance).where(
                        DeviceInstance.uuid == instance_element_uuid))
                instance = result.first()
                if not instance:
                    raise ProjectDBError(
                        f'Device with uuid "{instance_element_uuid}" not '
                        "found in the database. Cannot link the instance to "
                        f'server "{server.name}" ({server.uuid})')
                instance.device_server_id = server.id
                instance.order = instance_index
                instance_index += 1
                session.add(instance)

            await session.commit()

    async def add_domain(self, domain: str):
        if await self.domain_exists(domain):
            return
        """Add a domain to the project database"""
        async with self.session_gen() as session:
            project_domain = ProjectDomain(name=domain)
            session.add(project_domain)
            await session.commit()
