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

from lxml import etree
from sqlalchemy import func
from sqlalchemy.orm import sessionmaker
from sqlmodel import select

from .models import (
    DeviceConfig, DeviceInstance, DeviceServer, DeviceServerDeviceInstance,
    Macro, Project, ProjectDomain, ProjectSubproject, Scene)


class DbWriter:

    def __init__(self, session_gen: sessionmaker):
        self.session_gen = session_gen

    def save_project_item(self, domain: str, uuid: str, xml: str,
                          timestamp: str):
        prj = etree.fromstring(xml)
        date = datetime.datetime.strptime(timestamp, "%Y-%m-%d %H:%M:%S")

        # Save the project attributes
        project_id = None  # the id of the updated or new project
        with self.session_gen() as session:
            query = select(ProjectDomain).where(ProjectDomain.name == domain)
            project_domain = session.exec(query).first()
            # domain is expected to correspond to a valid domain.
            assert project_domain is not None

            query = select(Project).where(Project.uuid == uuid)
            project = session.exec(query).first()
            if not project:
                # There's still no record for the project in the DB
                project = Project(
                    uuid=uuid,
                    name=prj.attrib['simple_name'],
                    description=prj.attrib['description'],
                    trashed=prj.attrib['is_trashed'].lower() == "true",
                    date=date,
                    last_modified_user=prj.attrib['user'],
                    project_domain_id=project_domain.id)
            else:
                # There's a record for the project in the DB - update its
                # attributes from the data in the xml
                project.name = prj.attrib['simple_name']
                project.description = prj.attrib['description']
                # SQLmodel converts 'true' and 'false' strings to boolean
                # values successfuly on init, but not on an assignment - this
                # is the reason for the expression with the comparison to lower
                project.trashed = prj.attrib['is_trashed'].lower() == "true"
                project.date = date
                project.last_modified_user = prj.attrib['user']
                project.project_domain_id = project_domain.id

            session.add(project)
            # The commit / refresh sequence is needed for the case of a
            # new project. A new project only gets its ID after being
            # stored in the database.
            session.commit()
            session.refresh(project)

            project_id = project.id

            # Save the children references to the project and
            # their relative order. For each child type, the items of that
            # type currently linked to the project must be first unlinked
            # to the project.
            project_elem = prj.getchildren()[0].getchildren()[0]
            macros_elem = project_elem.getchildren()[0]
            scenes_elem = project_elem.getchildren()[1]
            servers_elem = project_elem.getchildren()[2]
            subprjs_elem = project_elem.getchildren()[3]

            query = select(Macro).where(Macro.project_id == project_id)
            curr_macros = session.exec(query).all()
            for curr_macro in curr_macros:
                curr_macro.project_id = None
                curr_macro.order = 0
                session.add(curr_macro)

            macro_idx = 0
            updated_macros = set()
            for macro_elem in macros_elem.getchildren():
                macro_uuid = macro_elem.getchildren()[0].text
                query = select(Macro).where(Macro.uuid == macro_uuid)
                macro = session.exec(query).first()
                if not macro:
                    raise RuntimeError(
                        f'Macro with uuid "{macro_uuid}" not found in the '
                        'database. Cannot link the macro to project '
                        f'"{project.name}" ({project.uuid})')
                updated_macros.add(macro_uuid)
                macro.project_id = project_id
                macro.order = macro_idx
                session.add(macro)
                macro_idx += 1
            # Removes any macro that was previously linked to the project, but
            # isn't anymore.
            for curr_macro in curr_macros:
                if curr_macro.uuid not in updated_macros:
                    session.delete(curr_macro)

            query = select(Scene).where(Scene.project_id == project_id)
            curr_scenes = session.exec(query).all()
            for curr_scene in curr_scenes:
                curr_scene.project_id = None
                curr_scene.order = 0
                session.add(curr_scene)

            scene_idx = 0
            updated_scenes = set()
            for scene_elem in scenes_elem.getchildren():
                scene_uuid = scene_elem.getchildren()[0].text
                query = select(Scene).where(Scene.uuid == scene_uuid)
                scene = session.exec(query).first()
                if not scene:
                    raise RuntimeError(
                        f'Scene with uuid "{scene_uuid}" not found in the '
                        'database. Cannot link the scene to project '
                        f'"{project.name}" ({project.uuid})')
                updated_scenes.add(scene_uuid)
                scene.project_id = project_id
                scene.order = scene_idx
                session.add(scene)
                scene_idx += 1
            # Remove any scene that was previously linked to the project but
            # isn't anymore.
            for curr_scene in curr_scenes:
                if curr_scene.uuid not in updated_scenes:
                    session.delete(curr_scene)

            query = select(DeviceServer).where(
                DeviceServer.project_id == project_id)
            curr_servers = session.exec(query).all()
            for curr_server in curr_servers:
                curr_server.project_id = None
                curr_server.order = 0
                session.add(curr_server)

            server_idx = 0
            updated_servers = set()
            for server_elem in servers_elem.getchildren():
                server_uuid = server_elem.getchildren()[0].text
                query = select(DeviceServer).where(
                    DeviceServer.uuid == server_uuid)
                server = session.exec(query).first()
                if not server:
                    raise RuntimeError(
                        f'Server with uuid "{server_uuid}" not found in the '
                        'database. Cannot link the server to project'
                        f'"{project.name}" ({project.uuid})')
                updated_servers.add(server_uuid)
                server.project_id = project_id
                server.order = server_idx
                session.add(server)
                server_idx += 1
            # Removes any device server that was previously linked to the
            # project but is not anymore
            for curr_server in curr_servers:
                if curr_server.uuid not in updated_servers:
                    # Before deleting the server, its relations to device
                    # instances must be cleared.
                    related_instances_ids = set()
                    query = select(DeviceServerDeviceInstance).where(
                        DeviceServerDeviceInstance.device_server_id ==
                        curr_server.id)
                    relations = session.exec(query).all()
                    for relation in relations:
                        related_instances_ids.add(relation.device_instance_id)
                        session.delete(relation)
                    # This commit is needed. With the transaction isolation
                    # level in the MySQL session, if the commit is omitted the
                    # checkings for remaining relations between device server's
                    # instances and any other server won't ever be zero and the
                    # instance (and its configs) won't be removed from the DB.
                    session.commit()
                    # If the instances associated to the about to be deleted
                    # server have no further association, they should also go
                    # (along with their associated configs)
                    # FIXME: remove the query for remaining relations (and
                    # the commit right above) once the aggregation of device
                    # instances by name is gone - then each device instance
                    # will be associated with a single device server. The
                    # DeviceServerDeviceInstance will be also removable, with
                    # the device_server_id becoming a nullable FK of
                    # DeviceInstance.
                    for related_instance_id in related_instances_ids:
                        query = select(
                            func.count(DeviceServerDeviceInstance.id)).where(
                                DeviceServerDeviceInstance.device_instance_id
                                == related_instance_id)
                        remaining = session.exec(query).one()
                        if remaining == 0:
                            query = select(DeviceConfig).where(
                                DeviceConfig.device_instance_id ==
                                related_instance_id)
                            related_configs = session.exec(query).all()
                            for related_config in related_configs:
                                session.delete(related_config)
                            query = select(DeviceInstance).where(
                                DeviceInstance.id == related_instance_id)
                            related_instance = session.exec(query).first()
                            if related_instance:
                                session.delete(related_instance)

                    # Now finally the server can go
                    session.delete(curr_server)

            query = select(ProjectSubproject).where(
                ProjectSubproject.project_id == project_id)
            curr_subprojects = session.exec(query).all()
            for curr_subproject in curr_subprojects:
                # this deletes only the relations between the project being
                # saved and its subprojects. The new relationships to be saved
                # contained in the xml will be added do the database in the
                # same transaction right after
                session.delete(curr_subproject)

            subproject_idx = 0
            for subprj_elem in subprjs_elem.getchildren():
                subprj_uuid = subprj_elem.getchildren()[0].text
                query = select(Project).where(
                    Project.uuid == subprj_uuid)
                subprj = session.exec(query).first()
                if not subprj:
                    raise RuntimeError(
                        f'Subproject with uuid "{server_uuid}" not found in '
                        'the database. Cannot link the subproject to project'
                        f'"{project.name}" ({project.uuid})')
                project_subproject = ProjectSubproject(
                    project_id=project_id,
                    subproject_id=subprj.id,
                    order=subproject_idx)
                session.add(project_subproject)
                subproject_idx += 1

            session.commit()

    def save_macro_item(self, uuid: str, xml: str, timestamp: str):
        import base64
        macro_obj = etree.fromstring(xml)
        macro_uuid = macro_obj.attrib["uuid"]
        macro_name = macro_obj.attrib["simple_name"]
        macro_user = macro_obj.attrib["user"]
        # In MySQL the macro bodies are not Base64 encoded
        macro_body = base64.b64decode(macro_obj.getchildren()[0].text)

        date = datetime.datetime.strptime(timestamp, "%Y-%m-%d %H:%M:%S")

        with self.session_gen() as session:
            query = select(Macro).where(Macro.uuid == uuid)
            macro = session.exec(query).first()
            if macro:
                # A macro is being updated
                macro.name = macro_name
                macro.date = date
                macro.last_modified_user = macro_user
                macro.body = macro_body
            else:
                # The macro is new
                macro = Macro(
                    uuid=macro_uuid,
                    name=macro_name,
                    date=date,
                    last_modified_user=macro_user,
                    body=macro_body)
            session.add(macro)
            session.commit()

    def save_scene_item(self, uuid: str, xml: str, timestamp: str):
        date = datetime.datetime.strptime(timestamp, "%Y-%m-%d %H:%M:%S")
        scene_obj = etree.fromstring(xml)
        scene_uuid = scene_obj.attrib["uuid"]
        scene_name = scene_obj.attrib["simple_name"]
        scene_user = scene_obj.attrib["user"]
        scene_svg_data = (
            etree.tostring(scene_obj.getchildren()[0]).decode("UTF-8")
            if len(scene_obj.getchildren()) > 0 else "")

        with self.session_gen() as session:
            query = select(Scene).where(Scene.uuid == uuid)
            scene = session.exec(query).first()
            if scene:
                # A scene is being updated
                scene.name = scene_name
                scene.date = date
                scene.last_modified_user = scene_user
                scene.svg_data = scene_svg_data
            else:
                # The scene is new
                scene = Scene(
                    uuid=scene_uuid,
                    name=scene_name,
                    date=date,
                    last_modified_user=scene_user,
                    svg_data=scene_svg_data)
            session.add(scene)
            session.commit()

    def save_device_config_item(self, uuid: str, xml: str, timestamp: str):
        date = datetime.datetime.strptime(timestamp, "%Y-%m-%d %H:%M:%S")
        config_obj = etree.fromstring(xml)
        config_uuid = config_obj.attrib["uuid"]
        config_name = config_obj.attrib["simple_name"]
        config_description = config_obj.attrib["description"]
        config_user = config_obj.attrib["user"]
        config_data = (
            etree.tostring(config_obj.getchildren()[0]).decode("UTF-8")
            if len(config_obj.getchildren()) > 0 else "")

        with self.session_gen() as session:
            query = select(DeviceConfig).where(DeviceConfig.uuid == uuid)
            config = session.exec(query).first()

            if config:
                # A device config is being updated
                config.name = config_name
                config.description = config_description
                config.config_data = config_data
                config.last_modified_user = config_user
                config.date = date
            else:
                # A new device config
                config = DeviceConfig(
                    uuid=config_uuid,
                    name=config_name,
                    config_data=config_data,
                    description=config_description,
                    last_modified_user=config_user,
                    date=date)

            session.add(config)
            session.commit()

    def save_device_instance_item(self, uuid: str, xml: str, timestamp: str):
        date = datetime.datetime.strptime(timestamp, "%Y-%m-%d %H:%M:%S")
        instance_obj = etree.fromstring(xml)
        instance_uuid = instance_obj.attrib["uuid"]
        instance_user = instance_obj.attrib["user"]
        instance_tag = instance_obj.getchildren()[0]
        instance_id = instance_tag.attrib['instance_id']
        instance_class_id = instance_tag.attrib['class_id']
        config_objs = instance_tag.getchildren()

        # Insert or update the device instance in the DB
        instance = None
        with self.session_gen() as session:
            query = select(DeviceInstance).where(DeviceInstance.uuid == uuid)
            instance = session.exec(query).first()
            if instance:
                # Updates the device instance
                instance.name = instance_id
                instance.class_id = instance_class_id
                instance.last_modified_user = instance_user
                instance.date = datetime.datetime.now(datetime.UTC)
            else:
                # The device instance must be added to the DB
                instance = DeviceInstance(
                    uuid=instance_uuid,
                    name=instance_id,
                    class_id=instance_class_id,
                    date=date,
                    last_modified_user=instance_user)
            session.add(instance)
            session.commit()
            session.refresh(instance)

            # Saves the device configs linked to the device instance that
            # has been just saved. First the currently linked configs
            # must be unlinked
            query = select(DeviceConfig).where(
                DeviceConfig.device_instance_id == instance.id)
            curr_configs = session.exec(query).all()
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
                config = session.exec(query).first()
                if not config:
                    raise RuntimeError(
                        f'Device config with uuid "{config_obj_uuid}" not '
                        "found in the database. Cannot link the config to "
                        f'instance "{instance.name}" ({instance.uuid})')
                config.device_instance_id = instance.id
                config.order = config_idx
                config_idx += 1
                session.add(config)
                updated_configs.add(config_obj_uuid)

            # Removes all configs that were linked to the instance in the
            # database, but that aren't anymore.
            for curr_config in curr_configs:
                if curr_config.uuid not in updated_configs:
                    session.delete(curr_config)

            session.commit()

    def save_device_server_item(self, uuid: str, xml: str, timestamp: str):
        date = datetime.datetime.strptime(timestamp, "%Y-%m-%d %H:%M:%S")
        server_obj = etree.fromstring(xml)
        server_uuid = server_obj.attrib["uuid"]
        server_name = server_obj.attrib["simple_name"]
        server_user = server_obj.attrib["user"]
        server_tag = server_obj.getchildren()[0]
        instance_objs = server_tag.getchildren()

        server = None
        with self.session_gen() as session:
            query = select(DeviceServer).where(DeviceServer.uuid == uuid)
            server = session.exec(query).first()

            if server:
                # An existing device server is being saved
                server.name = server_name
                server.last_modified_user = server_user
                server.date = date
            else:
                # A new device server is being saved
                server = DeviceServer(
                    uuid=server_uuid,
                    name=server_name,
                    date=date,
                    last_modified_user=server_user)

            session.add(server)
            session.commit()
            session.refresh(server)

            # Saves the device instances linked to the device server just saved
            # First unlinks all the currently linked device instances
            query = select(DeviceServerDeviceInstance).where(
                DeviceServerDeviceInstance.device_server_id == server.id)
            curr_linked_instances = session.exec(query).all()
            for curr_linked_instance in curr_linked_instances:
                # As the DeviceServerDeviceInstance only contains data about
                # the relation between a server and its instances (no attrib
                # of any of the two entities), we simply remove the relation
                # record.
                session.delete(curr_linked_instance)
            session.commit()

            instance_idx = 0
            for instance_obj in instance_objs:
                instance_obj_uuid = instance_obj.attrib["uuid"]
                query = select(DeviceInstance).where(
                    DeviceInstance.uuid == instance_obj_uuid)
                instance = session.exec(query).first()
                if not instance:
                    raise RuntimeError(
                        f'Device instance with uuid "{instance_obj_uuid}" not '
                        "found in the database. Cannot link the instance to "
                        f'server "{server.name}" ({server.uuid})')
                server_instance = DeviceServerDeviceInstance(
                    device_server_id=server.id,
                    device_instance_id=instance.id,
                    order=instance_idx)
                instance_idx += 1
                session.add(server_instance)

            session.commit()

            # Cleans up curr_instances that became orphans (along with their
            # configs). Any curr_linked_instance not linked with any device
            # server should be cleaned up.
            for curr_linked_instance in curr_linked_instances:
                query = select(
                    func.count(DeviceServerDeviceInstance.id)).where(
                        DeviceServerDeviceInstance.device_instance_id ==
                        curr_linked_instance.device_instance_id,
                        DeviceServerDeviceInstance.device_server_id ==
                        server.id)
                remaining = session.exec(query).one()
                if remaining == 0:
                    query = select(DeviceConfig).where(
                        DeviceConfig.device_instance_id ==
                        curr_linked_instance.device_instance_id)
                    orphan_configs = session.exec(query).all()
                    for orphan_config in orphan_configs:
                        session.delete(orphan_config)
                    query = select(DeviceInstance).where(
                        DeviceInstance.id ==
                        curr_linked_instance.device_instance_id)
                    orphan_instances = session.exec(query).all()
                    for orphan_instance in orphan_instances:
                        session.delete(orphan_instance)
                    session.commit()
