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

# DB data reading functions

import datetime
import logging

from lxml import etree
from sqlalchemy.orm import sessionmaker
from sqlmodel import select

from .models import (
    DeviceConfig, DeviceInstance, DeviceServer, Macro, Project, ProjectDomain,
    ProjectSubproject, Scene)

logger = logging.getLogger(__file__)


class DbWriter:

    def __init__(self, session_gen: sessionmaker, remove_orphans=False):
        self.session_gen = session_gen
        self.remove_orphans = remove_orphans

    async def register_project_load(self, project: Project):
        async with self.session_gen() as session:
            project.last_loaded = datetime.datetime.now(datetime.UTC)
            session.add(project)
            await session.commit()

    async def save_project_item(
            self, domain: str, uuid: str, xml: str, timestamp: str):
        prj = etree.fromstring(xml)
        date = datetime.datetime.fromisoformat(timestamp)

        # Save the project attributes
        project_id = None  # the id of the updated or new project
        async with self.session_gen() as session:
            query = select(ProjectDomain).where(ProjectDomain.name == domain)
            result = await session.exec(query)
            project_domain = result.first()
            # domain is expected to correspond to a valid domain.
            assert project_domain is not None

            query = select(Project).where(Project.uuid == uuid)
            result = await session.exec(query)
            project = result.first()
            if not project:
                # There's still no record for the project in the DB
                trashed = prj.attrib.get('is_trashed', '').lower() == "true"
                project = Project(
                    uuid=uuid,
                    name=prj.attrib['simple_name'],
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
                project.name = prj.attrib['simple_name']
                # SQLmodel converts 'true' and 'false' strings to boolean
                # values successfuly on init, but not on an assignment - this
                # is the reason for the expression with the comparison to lower
                project.is_trashed = prj.attrib['is_trashed'].lower() == "true"
                project.date = date
                project.project_domain_id = project_domain.id
                session.add(project)

            project_id = project.id

            # Save the children references to the project and
            # their relative order. For each child type, the items of that
            # type currently linked to the project must be first unlinked
            # to the project.
            project_elem = (
                prj.getchildren()[0].getchildren()[0]
                if (len(prj.getchildren()) > 0 and
                    len(prj.getchildren()[0].getchildren()) > 0)
                else None)
            macros_elem = None
            scenes_elem = None
            servers_elem = None
            subprjs_elem = None
            n_project_children = (
                len(project_elem.getchildren())
                if project_elem is not None else 0)
            for child_index in range(0, n_project_children):
                match project_elem.getchildren()[child_index].tag.lower():
                    case "macros":
                        macros_elem = project_elem.getchildren()[child_index]
                    case "scenes":
                        scenes_elem = project_elem.getchildren()[child_index]
                    case "servers":
                        servers_elem = project_elem.getchildren()[child_index]
                    case "subprojects":
                        subprjs_elem = project_elem.getchildren()[child_index]

            query = select(Macro).where(Macro.project_id == project_id)
            result = await session.exec(query)
            curr_macros = result.all()
            for curr_macro in curr_macros:
                curr_macro.project_id = None
                curr_macro.order = 0
                session.add(curr_macro)

            macro_idx = 0
            updated_macros = set()
            if macros_elem is not None:
                for macro_elem in macros_elem.getchildren():
                    macro_uuid = macro_elem.getchildren()[0].text
                    query = select(Macro).where(Macro.uuid == macro_uuid)
                    result = await session.exec(query)
                    macro = result.first()
                    if not macro:
                        logger.error(
                            f'Macro with uuid "{macro_uuid}" not found in the '
                            'database. Cannot link the macro to project '
                            f'"{project.name}" ({project.uuid})')
                        continue
                    updated_macros.add(macro_uuid)
                    macro.project_id = project_id
                    macro.order = macro_idx
                    session.add(macro)
                    macro_idx += 1
            # Removes any macro that was previously linked to the project, but
            # isn't anymore (depending on the remove_orphans option)
            if self.remove_orphans:
                for curr_macro in curr_macros:
                    if curr_macro.uuid not in updated_macros:
                        session.delete(curr_macro)

            query = select(Scene).where(Scene.project_id == project_id)
            result = await session.exec(query)
            curr_scenes = result.all()
            for curr_scene in curr_scenes:
                curr_scene.project_id = None
                curr_scene.order = 0
                session.add(curr_scene)

            scene_idx = 0
            updated_scenes = set()
            if scenes_elem is not None:
                for scene_elem in scenes_elem.getchildren():
                    scene_uuid = (
                        scene_elem.getchildren()[0].text
                        if len(scene_elem.getchildren()) > 0 else None)
                    # The legacy unit tests had the scene UUID as an attribute
                    # of a <xml> tag
                    if (scene_uuid is None
                            and "uuid" in scene_elem.attrib.keys()):
                        scene_uuid = scene_elem.attrib['uuid']
                    query = select(Scene).where(Scene.uuid == scene_uuid)
                    result = await session.exec(query)
                    scene = result.first()
                    if not scene:
                        logger.error(
                            f'Scene with uuid "{scene_uuid}" not found in the '
                            'database. Cannot link the scene to project '
                            f'"{project.name}" ({project.uuid})')
                        continue
                    updated_scenes.add(scene_uuid)
                    scene.project_id = project_id
                    scene.order = scene_idx
                    session.add(scene)
                    scene_idx += 1
            # Remove any scene that was previously linked to the project but
            # isn't anymore (depending on the remove_orphans option)
            if self.remove_orphans:
                for curr_scene in curr_scenes:
                    if curr_scene.uuid not in updated_scenes:
                        session.delete(curr_scene)

            query = select(DeviceServer).where(
                DeviceServer.project_id == project_id)
            result = await session.exec(query)
            curr_servers = result.all()
            for curr_server in curr_servers:
                curr_server.project_id = None
                curr_server.order = 0
                session.add(curr_server)

            server_idx = 0
            updated_servers = set()
            if servers_elem is not None:
                for server_elem in servers_elem.getchildren():
                    server_uuid = server_elem.getchildren()[0].text
                    query = select(DeviceServer).where(
                        DeviceServer.uuid == server_uuid)
                    result = await session.exec(query)
                    server = result.first()
                    if not server:
                        logger.error(
                            f'Server with uuid "{server_uuid}" not found in '
                            'the database. Cannot link the server to project'
                            f'"{project.name}" ({project.uuid})')
                        continue
                    updated_servers.add(server_uuid)
                    server.project_id = project_id
                    server.order = server_idx
                    session.add(server)
                    server_idx += 1
            # Removes any device server that was previously linked to the
            # project but is not anymore (depending on the remove_orphans
            # option)
            if self.remove_orphans:
                for curr_server in curr_servers:
                    if curr_server.uuid not in updated_servers:
                        # Before deleting the server, its device instances must
                        # be deleted - after deleting those instances configs.
                        query = select(DeviceInstance).where(
                            DeviceInstance.device_server_id == curr_server.id)
                        result = await session.exec(query)
                        instances = result.all()
                        for instance in instances:
                            # Delete the instances configs
                            query = select(DeviceConfig).where(
                                DeviceConfig.device_instance_id == instance.id)
                            result = await session.exec(query)
                            related_configs = result.all()
                            for related_config in related_configs:
                                session.delete(related_config)
                            query = select(DeviceInstance).where(
                                DeviceInstance.id == instance.id)
                            result = await session.exec(query)
                            related_instance = result.first()
                            if related_instance:
                                session.delete(related_instance)
                        # Now finally the server can go
                        session.delete(curr_server)

            query = select(ProjectSubproject).where(
                ProjectSubproject.project_id == project_id)
            result = await session.exec(query)
            curr_subprojects = result.all()

            subproject_idx = 0
            # Initially all the previously saved subproject are to be
            # potentially removed
            # Note: subprojects are only a reference to projects, not real
            #       entities stored in the database like scenes and macros.
            #       There's no need to handle remove_orphans for them as they
            #       are updated on single save_item roundtrip between the
            #       GUI Client and the Project Manager
            subprojects_to_delete = {
                subproject.subproject_id for subproject in curr_subprojects}
            # Iterate over the subprojects to be saved
            if subprjs_elem is not None:
                for subprj_elem in subprjs_elem.getchildren():
                    subprj_uuid = subprj_elem.getchildren()[0].text
                    query = select(Project).where(
                        Project.uuid == subprj_uuid)
                    result = await session.exec(query)
                    subprj = result.first()
                    if not subprj:
                        logger.error(
                            f'Subproject with uuid "{server_uuid}" not found '
                            'in the database. Cannot link the subproject to '
                            f'project "{project.name}" ({project.uuid})')
                        continue
                    # Search for the subproject among the current projects, and
                    # either add it, update it or leave it in the list of
                    # subproject to be removed
                    if subprj.id in subprojects_to_delete:
                        # The project was in the DB and should be kept - just
                        # update it
                        subprojects_to_delete.remove(subprj.id)
                        query = select(ProjectSubproject).where(
                            ProjectSubproject.project_id == project_id,
                            ProjectSubproject.subproject_id == subprj.id)
                        result = await session.exec(query)
                        prj_subprj = result.first()
                        if prj_subprj:
                            prj_subprj.order = subproject_idx
                        session.add(prj_subprj)
                    else:
                        # The project wasn't in the DB - add it
                        prj_subprj = ProjectSubproject(
                            project_id=project_id,
                            subproject_id=subprj.id,
                            order=subproject_idx)
                        session.add(prj_subprj)
                    subproject_idx += 1

            # Remove the subprojects that were in the DB before but shouldn't
            # be there after the save
            for subprj_id in subprojects_to_delete:
                query = select(ProjectSubproject).where(
                    ProjectSubproject.project_id == project_id,
                    ProjectSubproject.subproject_id == subprj_id)
                result = await session.exec(query)
                subprj_to_delete = result.first()
                if subprj_to_delete:
                    session.delete(subprj_to_delete)

            await session.commit()

    async def save_macro_item(self, uuid: str, xml: str, timestamp: str):
        import base64
        macro_obj = etree.fromstring(xml)
        macro_uuid = macro_obj.attrib["uuid"]
        macro_name = macro_obj.attrib["simple_name"]
        # In MySQL the macro bodies are not Base64 encoded
        macro_body = base64.b64decode(macro_obj.getchildren()[0].text)

        date = datetime.datetime.fromisoformat(timestamp)

        async with self.session_gen() as session:
            query = select(Macro).where(Macro.uuid == uuid)
            result = await session.exec(query)
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

    async def save_scene_item(self, uuid: str, xml: str, timestamp: str):
        date = datetime.datetime.fromisoformat(timestamp)
        scene_obj = etree.fromstring(xml)
        scene_uuid = scene_obj.attrib["uuid"]
        scene_name = scene_obj.attrib["simple_name"]
        scene_svg_data = (
            etree.tostring(scene_obj.getchildren()[0]).decode("UTF-8")
            if len(scene_obj.getchildren()) > 0 else "")

        async with self.session_gen() as session:
            query = select(Scene).where(Scene.uuid == uuid)
            result = await session.exec(query)
            scene = result.first()
            if scene:
                # A scene is being updated
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

    async def save_device_config_item(
            self, uuid: str, xml: str, timestamp: str):
        date = datetime.datetime.fromisoformat(timestamp)
        config_obj = etree.fromstring(xml)
        config_uuid = config_obj.attrib["uuid"]
        config_name = config_obj.attrib["simple_name"]
        config_data = (
            etree.tostring(config_obj.getchildren()[0]).decode("UTF-8")
            if len(config_obj.getchildren()) > 0 else "")

        async with self.session_gen() as session:
            query = select(DeviceConfig).where(DeviceConfig.uuid == uuid)
            result = await session.exec(query)
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

    async def save_device_instance_item(
            self, uuid: str, xml: str, timestamp: str):
        date = datetime.datetime.fromisoformat(timestamp)
        instance_obj = etree.fromstring(xml)
        instance_uuid = instance_obj.attrib["uuid"]
        instance_tag = instance_obj.getchildren()[0]
        instance_id = instance_tag.attrib['instance_id']
        instance_class_id = instance_tag.attrib['class_id']
        instance_active_uuid = instance_tag.attrib['active_uuid']
        config_objs = instance_tag.getchildren()

        # Insert or update the device instance in the DB
        instance = None
        async with self.session_gen() as session:
            query = select(DeviceInstance).where(DeviceInstance.uuid == uuid)
            result = await session.exec(query)
            instance = result.first()
            if instance:
                # Updates the device instance
                instance.name = instance_id
                instance.class_id = instance_class_id
                instance.date = date
                session.add(instance)
            else:
                # The device instance must be added to the DB
                instance = DeviceInstance(
                    uuid=instance_uuid,
                    name=instance_id,
                    class_id=instance_class_id,
                    date=date)
                session.add(instance)
                await session.commit()
                await session.refresh(instance)

            # Saves the device configs linked to the device instance that
            # has been just saved. First the currently linked configs
            # must be unlinked
            query = select(DeviceConfig).where(
                DeviceConfig.device_instance_id == instance.id)
            result = await session.exec(query)
            curr_configs = result.all()
            for curr_config in curr_configs:
                curr_config.device_instance_id = None
                curr_config.order = 0
                session.add(curr_config)

            config_idx = 0
            updated_configs = set()
            for config_obj in config_objs:
                config_obj_uuid = config_obj.attrib["uuid"]
                query = select(DeviceConfig).where(
                    DeviceConfig.uuid == config_obj_uuid)
                result = await session.exec(query)
                config = result.first()
                if not config:
                    raise RuntimeError(
                        f'Device config with uuid "{config_obj_uuid}" not '
                        "found in the database. Cannot link the config to "
                        f'instance "{instance.name}" ({instance.uuid})')
                config.device_instance_id = instance.id
                config.order = config_idx
                config.is_active = (instance_active_uuid == config_obj_uuid)
                config_idx += 1
                session.add(config)
                updated_configs.add(config_obj_uuid)

            # Removes all configs that were linked to the instance in the
            # database, but that aren't anymore (depending on the
            # remove_orphans option)
            if self.remove_orphans:
                for curr_config in curr_configs:
                    if curr_config.uuid not in updated_configs:
                        session.delete(curr_config)

            await session.commit()

    async def save_device_server_item(
            self, uuid: str, xml: str, timestamp: str):
        date = datetime.datetime.fromisoformat(timestamp)
        server_obj = etree.fromstring(xml)
        server_uuid = server_obj.attrib["uuid"]
        server_name = server_obj.attrib["simple_name"]
        server_tag = (
            server_obj.getchildren()[0]
            if len(server_obj.getchildren()) > 0 else None)
        instance_objs = (
            server_tag.getchildren()
            if server_tag is not None
            else [])

        server = None
        async with self.session_gen() as session:
            query = select(DeviceServer).where(DeviceServer.uuid == uuid)
            result = await session.exec(query)
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
            query = select(DeviceInstance).where(
                DeviceInstance.device_server_id == server.id)
            result = await session.exec(query)
            curr_linked_instances = result.all()
            for curr_linked_instance in curr_linked_instances:
                curr_linked_instance.device_server_id = None
                curr_linked_instance.order = 0
                session.add(curr_linked_instance)

            instance_idx = 0
            updated_instances = set()
            for instance_obj in instance_objs:
                instance_obj_uuid = instance_obj.attrib["uuid"]
                query = select(DeviceInstance).where(
                    DeviceInstance.uuid == instance_obj_uuid)
                result = await session.exec(query)
                instance = result.first()
                if not instance:
                    raise RuntimeError(
                        f'Device instance with uuid "{instance_obj_uuid}" not '
                        "found in the database. Cannot link the instance to "
                        f'server "{server.name}" ({server.uuid})')
                updated_instances.add(instance.id)
                instance.device_server_id = server.id
                instance.order = instance_idx
                instance_idx += 1
                session.add(instance)

            # Removes any instance that is not linked to the device server
            # anymore - all the curr_linked_instances whose ids are not in
            # the updated_instances set (depending on the remove_orphans
            # option)
            if self.remove_orphans:
                for curr_linked_instance in curr_linked_instances:
                    if curr_linked_instance.id not in updated_instances:
                        session.delete(curr_linked_instance)

            await session.commit()

    async def add_domain(self, domain: str):
        """Add a domain to the project database"""
        async with self.session_gen() as session:
            project_domain = ProjectDomain(name=domain)
            session.add(project_domain)
            await session.commit()
