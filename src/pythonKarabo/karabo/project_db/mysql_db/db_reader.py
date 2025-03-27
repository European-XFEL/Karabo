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

from sqlalchemy.orm import sessionmaker
from sqlmodel import column, select

from .models import (
    DeviceConfig, DeviceInstance, DeviceServer, Macro, Project, ProjectDomain,
    ProjectSubproject, Scene)


class DbReader:

    def __init__(self, session_gen: sessionmaker):
        self.session_gen = session_gen

    def get_domain_projects(self, domain: str) -> list[dict[str, any]]:
        projects = []
        with self.session_gen() as session:
            query = select(Project).join(ProjectDomain).where(
                ProjectDomain.name == domain)
            domain_projects = session.exec(query).all()
            for project in domain_projects:
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

    def get_domain_macros(self, domain: str) -> list[dict[str, any]]:
        macros = []
        with self.session_gen() as session:
            query = (
                select(Macro).join(Project).join(ProjectDomain)
                .where(ProjectDomain.name == domain))
            domain_macros = session.exec(query).all()
            for macro in domain_macros:
                macros.append({
                    "uuid": macro.uuid,
                    "item_type": "macro",
                    "simple_name": macro.name
                })
        return macros

    def get_domain_scenes(self, domain: str) -> list[dict[str, any]]:
        scenes = []
        with self.session_gen() as session:
            query = (
                select(Scene).join(Project).join(ProjectDomain)
                .where(ProjectDomain.name == domain))
            domain_scenes = session.exec(query).all()
            for scene in domain_scenes:
                scenes.append({
                    "uuid": scene.uuid,
                    "item_type": "scene",
                    "simple_name": scene.name
                })
        return scenes

    def get_domain_device_servers(self, domain: str) -> list[dict[str, any]]:
        servers = []
        with self.session_gen() as session:
            query = (
                select(DeviceServer).join(Project).join(ProjectDomain)
                .where(ProjectDomain.name == domain))
            device_servers = session.exec(query).all()
            for server in device_servers:
                servers.append({
                    "uuid": server.uuid,
                    "item_type": "device_server",
                    "simple_name": server.name
                })
        return servers

    def get_domain_device_instances(self, domain: str) -> list[dict[str, any]]:
        instances = []
        with self.session_gen() as session:
            query = (
                select(DeviceInstance)
                .join(DeviceServer).join(Project).join(ProjectDomain)
                .where(ProjectDomain.name == domain))
            device_instances = session.exec(query).all()
            for instance in device_instances:
                instances.append({
                    "uuid": instance.uuid,
                    "item_type": "device_instance",
                    "simple_name": instance.name
                })
        return instances

    def get_domain_device_configs(self, domain: str) -> list[dict[str, any]]:
        configs = []
        with self.session_gen() as session:
            query = (
                select(DeviceConfig)
                .join(DeviceInstance).join(DeviceServer).join(Project)
                .join(ProjectDomain).where(ProjectDomain.name == domain))
            device_configs = session.exec(query).all()
            for config in device_configs:
                configs.append({
                    "uuid": config.uuid,
                    "item_type": "device_config",
                    "simple_name": config.name
                })
        return configs

    def get_subprojects_of_project(self, project: Project) -> list[Project]:
        subprojects = []
        with self.session_gen() as session:
            query = select(ProjectSubproject).where(
                ProjectSubproject.project_id == project.id).order_by(
                    ProjectSubproject.order)
            subprjs = session.exec(query).all()
            for subprj in subprjs:
                query_subprj = select(Project).where(
                    Project.id == subprj.subproject_id)
                subprj_rec = session.exec(query_subprj).first()
                if subprj_rec:
                    subprojects.append(subprj_rec)
        return subprojects

    def get_scenes_of_project(self, project: Project) -> list[Scene]:
        scenes = []
        with self.session_gen() as session:
            query = select(Scene).where(
                Scene.project_id == project.id).order_by(
                    Scene.order)
            scenes = session.exec(query).all()
        return scenes

    def get_macros_of_project(self, project: Project) -> list[Macro]:
        macros = []
        with self.session_gen() as session:
            query = select(Macro).where(
                Macro.project_id == project.id).order_by(
                    Macro.order)
            macros = session.exec(query).all()
        return macros

    def get_device_servers_of_project(self,
                                      project: Project) -> list[DeviceServer]:
        servers = []
        with self.session_gen() as session:
            query = select(DeviceServer).where(
                DeviceServer.project_id == project.id).order_by(
                    DeviceServer.order)
            servers = session.exec(query).all()
        return servers

    def get_device_instances_of_server(
            self, server: DeviceServer) -> list[DeviceInstance]:
        instances = []
        with self.session_gen() as session:
            query = select(DeviceInstance).where(
                DeviceInstance.device_server_id == server.id).order_by(
                    DeviceInstance.order)
            instances = session.exec(query).all()
        return instances

    def get_device_configs_of_instance(
            self, instance: DeviceInstance) -> list[DeviceConfig]:
        configs = []
        with self.session_gen() as session:
            query = select(DeviceConfig).where(
                DeviceConfig.device_instance_id == instance.id).order_by(
                    DeviceConfig.order)
            configs = session.exec(query).all()
        return configs

    def get_project_from_uuid(self, uuid: str) -> Project | None:
        project = None
        with self.session_gen() as session:
            query = select(Project).where(
                    Project.uuid == uuid)
            project = session.exec(query).first()
        return project

    def get_scene_from_uuid(self, uuid: str) -> Scene | None:
        scene = None
        with self.session_gen() as session:
            query = select(Scene).where(Scene.uuid == uuid)
            scene = session.exec(query).first()
        return scene

    def get_macro_from_uuid(self, uuid: str) -> Macro | None:
        macro = None
        with self.session_gen() as session:
            query = select(Macro).where(Macro.uuid == uuid)
            macro = session.exec(query).first()
        return macro

    def get_device_server_from_uuid(self, uuid: str) -> DeviceServer | None:
        server = None
        with self.session_gen() as session:
            query = select(DeviceServer).where(DeviceServer.uuid == uuid)
            server = session.exec(query).first()
        return server

    def get_device_instance_from_uuid(self,
                                      uuid: str) -> DeviceInstance | None:
        instance = None
        with self.session_gen() as session:
            query = select(DeviceInstance).where(DeviceInstance.uuid == uuid)
            instance = session.exec(query).first()
        return instance

    def get_device_config_from_uuid(self, uuid: str) -> DeviceConfig | None:
        config = None
        with self.session_gen() as session:
            query = select(DeviceConfig).where(DeviceConfig.uuid == uuid)
            config = session.exec(query).first()
        return config

    def get_domain_device_instances_by_name_part(
            self, domain: str, name_part: str) -> list[DeviceInstance]:
        with self.session_gen() as session:
            query = select(DeviceInstance).filter(
                column("name").contains(name_part))
            instances_by_name_part = session.exec(query).all()
            # Only keep the instances linked to projects in the domain
            instances_in_domain = []
            for instance in instances_by_name_part:
                device_server = instance.device_server
                if device_server:
                    project = device_server.project
                    if project:
                        project_domain = project.project_domain
                        if project_domain.name == domain:
                            instances_in_domain.append(instance)
            return instances_in_domain

    def get_device_instance_project(
            self, instance: DeviceInstance) -> Project | None:
        import datetime
        project = None
        with self.session_gen() as session:
            query = select(DeviceServer).where(
                DeviceServer.id == instance.device_server_id)
            device_server = session.exec(query).first()
            if device_server:
                project: Project = device_server.project
                # Add tzinfo to the project dates (naive datetimes stored in
                # in the DB)
                if project.date:
                    project.date = project.date.replace(tzinfo=datetime.UTC)
                if project.last_loaded:
                    project.last_loaded = project.last_loaded.replace(
                        tzinfo=datetime.UTC)
        return project
