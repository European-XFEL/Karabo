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
from sqlmodel import select

from .models import (
    DeviceConfig, DeviceInstance, DeviceServer, DeviceServerDeviceInstance,
    Macro, Project, ProjectDomain, ProjectSubproject, Scene)


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
                    "is_trashed": project.trashed,
                    "date": str(project.date),
                    "user": project.last_modified_user,
                    "description": project.description
                }
                projects.append(prj_dict)
        return projects

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
            query = select(DeviceInstance).join(
                DeviceServerDeviceInstance).where(
                    DeviceServerDeviceInstance.device_server_id ==
                    server.id).order_by(DeviceServerDeviceInstance.order)
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
