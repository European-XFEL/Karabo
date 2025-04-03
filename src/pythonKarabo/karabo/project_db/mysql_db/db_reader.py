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

from sqlalchemy.orm import sessionmaker
from sqlmodel import select

from .models import (
    DeviceConfig, DeviceInstance, DeviceServer, Macro, Project, ProjectDomain,
    ProjectSubproject, Scene)


class DbReader:

    def __init__(self, session_gen: sessionmaker):
        self.session_gen = session_gen

    async def get_domain_projects(self, domain: str) -> list[dict[str, any]]:
        projects = []
        async with self.session_gen() as session:
            query = select(Project).join(ProjectDomain).where(
                ProjectDomain.name == domain)
            domain_projects = await session.exec(query)
            result = domain_projects.all()
            for project in result:
                prj_dict = {
                    "uuid": project.uuid,
                    "item_type": "project",
                    "simple_name": project.name,
                    # NOTE: The GUI Client compares the is_trashed value
                    #       with the constants 'true' and 'false' (all lower)
                    "is_trashed": "true" if project.is_trashed else "false",
                    "date": (str(project.date)
                             if project.date is not None else ''),
                    "user": project.last_modified_user,
                    "description": project.description
                }
                projects.append(prj_dict)

        return projects

    async def get_domain_macros(self, domain: str) -> list[dict[str, any]]:
        macros = []
        async with self.session_gen() as session:
            query = (
                select(Macro).join(Project).join(ProjectDomain)
                .where(ProjectDomain.name == domain))
            result = await session.exec(query)
            domain_macros = result.all()
            for macro in domain_macros:
                macros.append({
                    "uuid": macro.uuid,
                    "item_type": "macro",
                    "simple_name": macro.name
                })
        return macros

    async def get_domain_scenes(self, domain: str) -> list[dict[str, any]]:
        scenes = []
        async with self.session_gen() as session:
            query = (
                select(Scene).join(Project).join(ProjectDomain)
                .where(ProjectDomain.name == domain))
            result = await session.exec(query)
            domain_scenes = result.all()
            for scene in domain_scenes:
                scenes.append({
                    "uuid": scene.uuid,
                    "item_type": "scene",
                    "simple_name": scene.name
                })
        return scenes

    async def get_domain_device_servers(
            self, domain: str) -> list[dict[str, any]]:
        servers = []
        async with self.session_gen() as session:
            query = (
                select(DeviceServer).join(Project).join(ProjectDomain)
                .where(ProjectDomain.name == domain))
            result = await session.exec(query)
            device_servers = result.all()
            for server in device_servers:
                servers.append({
                    "uuid": server.uuid,
                    "item_type": "device_server",
                    "simple_name": server.name
                })
        return servers

    async def get_domain_device_instances(
            self, domain: str) -> list[dict[str, any]]:
        instances = []
        async with self.session_gen() as session:
            query = (
                select(DeviceInstance)
                .join(DeviceServer).join(Project).join(ProjectDomain)
                .where(ProjectDomain.name == domain))
            result = await session.exec(query)
            device_instances = result.all()
            for instance in device_instances:
                instances.append({
                    "uuid": instance.uuid,
                    "item_type": "device_instance",
                    "simple_name": instance.name
                })
        return instances

    async def get_domain_device_configs(
            self, domain: str) -> list[dict[str, any]]:
        configs = []
        async with self.session_gen() as session:
            query = (
                select(DeviceConfig)
                .join(DeviceInstance).join(DeviceServer).join(Project)
                .join(ProjectDomain).where(ProjectDomain.name == domain))
            result = await session.exec(query)
            device_configs = result.all()
            for config in device_configs:
                configs.append({
                    "uuid": config.uuid,
                    "item_type": "device_config",
                    "simple_name": config.name
                })
        return configs

    async def get_subprojects_of_project(
            self, project: Project) -> list[Project]:
        subprojects = []
        async with self.session_gen() as session:
            query = select(ProjectSubproject).where(
                ProjectSubproject.project_id == project.id).order_by(
                    ProjectSubproject.order)
            result = await session.exec(query)
            subprjs = result.all()
            for subprj in subprjs:
                query_subprj = select(Project).where(
                    Project.id == subprj.subproject_id)
                result = await session.exec(query_subprj)
                subprj_rec = result.first()
                if subprj_rec:
                    subprojects.append(subprj_rec)
        return subprojects

    async def get_scenes_of_project(self, project: Project) -> list[Scene]:
        scenes = []
        async with self.session_gen() as session:
            query = select(Scene).where(
                Scene.project_id == project.id).order_by(
                    Scene.order)
            result = await session.exec(query)
            scenes = result.all()
        return scenes

    async def get_macros_of_project(self, project: Project) -> list[Macro]:
        macros = []
        async with self.session_gen() as session:
            query = select(Macro).where(
                Macro.project_id == project.id).order_by(
                    Macro.order)
            result = await session.exec(query)
            macros = result.all()
        return macros

    async def get_device_servers_of_project(
            self, project: Project) -> list[DeviceServer]:
        servers = []
        async with self.session_gen() as session:
            query = select(DeviceServer).where(
                DeviceServer.project_id == project.id).order_by(
                    DeviceServer.order)
            result = await session.exec(query)
            servers = result.all()
        return servers

    async def get_device_instances_of_server(
            self, server: DeviceServer) -> list[DeviceInstance]:
        instances = []
        async with self.session_gen() as session:
            query = select(DeviceInstance).where(
                DeviceInstance.device_server_id == server.id).order_by(
                    DeviceInstance.order)
            result = await session.exec(query)
            instances = result.all()
        return instances

    async def get_device_configs_of_instance(
            self, instance: DeviceInstance) -> list[DeviceConfig]:
        configs = []
        async with self.session_gen() as session:
            query = select(DeviceConfig).where(
                DeviceConfig.device_instance_id == instance.id).order_by(
                    DeviceConfig.order)
            result = await session.exec(query)
            configs = result.all()
        return configs

    async def get_project_from_uuid(self, uuid: str) -> Project | None:
        project = None
        async with self.session_gen() as session:
            query = select(Project).where(
                    Project.uuid == uuid)
            result = await session.exec(query)
            project = result.first()
        return project

    async def get_scene_from_uuid(self, uuid: str) -> Scene | None:
        scene = None
        async with self.session_gen() as session:
            query = select(Scene).where(Scene.uuid == uuid)
            result = await session.exec(query)
            scene = result.first()
        return scene

    async def get_macro_from_uuid(self, uuid: str) -> Macro | None:
        macro = None
        async with self.session_gen() as session:
            query = select(Macro).where(Macro.uuid == uuid)
            result = await session.exec(query)
            macro = result.first()
        return macro

    async def get_device_server_from_uuid(
            self, uuid: str) -> DeviceServer | None:
        server = None
        async with self.session_gen() as session:
            query = select(DeviceServer).where(DeviceServer.uuid == uuid)
            result = await session.exec(query)
            server = result.first()
        return server

    async def get_device_instance_from_uuid(
            self, uuid: str) -> DeviceInstance | None:
        instance = None
        async with self.session_gen() as session:
            query = select(DeviceInstance).where(DeviceInstance.uuid == uuid)
            result = await session.exec(query)
            instance = result.first()
        return instance

    async def get_device_config_from_uuid(
            self, uuid: str) -> DeviceConfig | None:
        config = None
        async with self.session_gen() as session:
            query = select(DeviceConfig).where(DeviceConfig.uuid == uuid)
            result = await session.exec(query)
            config = result.first()

        return config

    async def get_domain_device_instances_by_name_part(
            self, domain: str, name_part: str) -> list[DeviceInstance]:
        async with self.session_gen() as session:
            query = (
                select(DeviceInstance)
                .join(DeviceServer).join(Project).join(ProjectDomain)
                .filter(DeviceInstance.name.like(f'%{name_part}%'))
                .where(ProjectDomain.name == domain))
            result = await session.exec(query)
            instances_by_name_part = result.all()
            return instances_by_name_part

    async def get_device_instance_project(
            self, instance: DeviceInstance) -> Project | None:
        project = None
        async with self.session_gen() as session:
            query = select(Project).join(DeviceServer).where(
                DeviceServer.id == instance.device_server_id)
            result = await session.exec(query)
            project = result.first()
            if project:
                # Add tzinfo to the project dates (naive datetimes stored in
                # in the DB)
                if project.date:
                    project.date = project.date.replace(tzinfo=datetime.UTC)
                if project.last_loaded:
                    project.last_loaded = project.last_loaded.replace(
                        tzinfo=datetime.UTC)
        return project
