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

from sqlmodel import SQLModel, select

from ..bases import DatabaseBase, HandleABC
from ..util import ProjectDBError
from .db_engine import init_db_engine
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
    # region HandleABC compliance

    def hasCollection(self, path):
        return False

    def removeCollection(self, path):
        pass

    def createCollection(self, path):
        pass

    def hasDocument(self, path):
        return False

    def load(self, data, path):
        pass

    def getDoc(self, name):
        return None

    def getDocument(self, name):
        return None

    # end region


class ProjectDatabase(DatabaseBase):

    def __init__(self, user: str, password: str,
                 server: str, port: int, db_name: str):
        super().__init__()
        (self.db_engine, self.session_gen) = (
            init_db_engine(user, password, server, port, db_name))
        self.reader = DbReader(self.session_gen)
        self.writer = DbWriter(self.session_gen)
        self.metadata_created = False

    def onEnter(self):
        if not self.metadata_created:
            SQLModel.metadata.create_all(self.db_engine, checkfirst=True)
            self.metadata_created = True
        return MySQLHandle()

    def list_domains(self) -> list[str]:
        domains = []
        with self.session_gen() as session:
            query = select(ProjectDomain)
            prj_domains = session.exec(query).all()
            for domain in prj_domains:
                domains.append(domain.name)
        return domains

    def list_items(self, domain: str, item_types=None) -> list[dict[str, any]]:
        if item_types and len(item_types) == 1 and item_types[0] == 'project':
            return self.reader.get_domain_projects(domain)

    def load_item(self,
                  domain: str, items_uuids: list[str]) -> list[dict[str, any]]:
        loaded_items = []
        for item_uuid in items_uuids:
            # Retrieve the appropriate model from its UUID
            project = self.reader.get_project_from_uuid(item_uuid)
            if project:
                subprojects = self.reader.get_subprojects_of_project(project)
                scenes = self.reader.get_scenes_of_project(project)
                macros = self.reader.get_macros_of_project(project)
                servers = self.reader.get_device_servers_of_project(project)
                loaded_items.append({
                    "uuid": item_uuid,
                    "xml": emit_project_xml(
                        project, scenes, macros, servers, subprojects)})
                self.writer.register_project_load(project)
                continue
            scene = self.reader.get_scene_from_uuid(item_uuid)
            if scene:
                loaded_items.append({
                    "uuid": item_uuid,
                    "xml": emit_scene_xml(scene)})
                continue
            macro = self.reader.get_macro_from_uuid(item_uuid)
            if macro:
                loaded_items.append({
                    "uuid": item_uuid,
                    "xml": emit_macro_xml(macro)})
                continue
            server = self.reader.get_device_server_from_uuid(item_uuid)
            if server:
                instances = self.reader.get_device_instances_of_server(server)
                loaded_items.append({
                    "uuid": item_uuid,
                    "xml": emit_device_server_xml(server, instances)})
                continue
            instance = self.reader.get_device_instance_from_uuid(item_uuid)
            if instance:
                configs = self.reader.get_device_configs_of_instance(instance)
                loaded_items.append({
                    "uuid": item_uuid,
                    "xml": emit_device_instance_xml(instance, configs)})
                continue
            config = self.reader.get_device_config_from_uuid(item_uuid)
            if config:
                loaded_items.append({
                    "uuid": item_uuid,
                    "xml": emit_device_config_xml(config)})
                continue
        return loaded_items

    def _check_for_modification(self, uuid: str,
                                old_date: str, item_type: str):
        """ Check whether the item with of the given `domain` and `uuid` was
        modified

        :param uuid: the item's uuid
        :param old_date: the item's last modified time stamp
        :param item_type: the type of the item to be checked (e.g. 'project')
        :return A tuple stating whether the item was modified inbetween and a
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

    def save_item(self, domain, uuid, item_xml, overwrite=False):
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
        :param overwrite: defaults to False. If set to True versioning
                          information is removed prior to database injection,
                          allowing to overwrite in case of versioning
                          conflicts.

        :return: (True, dict) if successful, (false, dict) with dict
                 representing the item's versioning information. In case of
                 version conflict this will contain the information of the
                 *newest existing* entry in the database. If saving was
                 successful (no conflict), it will contain the updated
                 versioning information after saving the item.


        :raises: ProjectDBError on Handle Failure.
            RuntimeError if item saving failed otherwise.
            AttributeError if a non-supported type is passed
        """

        with self.session_gen() as session:
            query = select(ProjectDomain).where(ProjectDomain.name == domain)
            project_domain = session.exec(query).first()
            if project_domain is None:
                raise ProjectDBError(
                    f"Domain {domain} does not exist in the DB. Item won't "
                    "be saved.")

        # Extract some information
        try:
            # NOTE: The client might send us garbage
            item_tree = self._make_xml_if_needed(item_xml)
        except ValueError:
            msg = f'XML parse error for item "{uuid}"'
            raise ProjectDBError(msg)

        item_type = item_tree.attrib.get('item_type', None)

        if 'user' not in item_tree.attrib:
            item_tree.attrib['user'] = 'Karabo User'
        if not item_tree.attrib.get('date'):
            timestamp = datetime.datetime.now(datetime.UTC).isoformat(
                timespec="seconds")
            # timestamp = strftime(DATE_FORMAT, gmtime())
            item_tree.attrib['date'] = timestamp
        else:
            modified, reason = self._check_for_modification(
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

        item_xml = self._make_str_if_needed(item_tree)

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

    def get_configurations_from_device_name_part(self, domain, device_id_part):
        """
        Returns a list of configurations for a given device
        :param domain: DB domain
        :param device_id_part: part of device name; search is case-insensitive.
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
                results.append(
                    {"config_id": config.uuid,
                     "device_uuid": instance.uuid,
                     "device_id": instance.name})
        return results

    def get_projects_data_from_device(
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
