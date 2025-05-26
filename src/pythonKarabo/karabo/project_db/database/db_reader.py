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

from sqlalchemy.orm import sessionmaker
from sqlmodel import select

from .models import (
    DeviceConfig, DeviceInstance, DeviceServer, Macro, Project, ProjectDomain,
    ProjectSubproject, Scene)


def set_project_timezone(project: Project):
    """Set timezone of the timestamps of the project item"""
    if project.date:
        project.date = project.date.replace(
            tzinfo=datetime.UTC)
    if project.last_loaded:
        project.last_loaded = project.last_loaded.replace(
            tzinfo=datetime.UTC)


class DbReader:
    def __init__(self, session_gen: sessionmaker):
        self.session_gen = session_gen

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

    async def list_domains(self):
        return [d.name for d in await self._execute_all(select(ProjectDomain))]

    async def get_domain_projects(self, domain: str):
        query = select(Project).join(ProjectDomain).where(
            ProjectDomain.name == domain)
        result = await self._execute_all(query)
        return [{
            "uuid": p.uuid,
            "item_type": "project",
            "simple_name": p.name,
            # NOTE: The GUI Client compares the is_trashed value
            #       with the constants 'true' and 'false' (all lower)
            "is_trashed": "true" if p.is_trashed else "false",
            "date": str(p.date) if p.date else '',
            "user": p.last_modified_user,
            "description": p.description
        } for p in result]

    async def _get_named_items(self, domain: str, model, item_type: str):
        query = (
            select(model)
            .join(Project).join(ProjectDomain)
            .where(ProjectDomain.name == domain)
        )
        items = await self._execute_all(query)
        return [{"uuid": i.uuid, "item_type": item_type, "simple_name": i.name}
                for i in items]

    async def get_domain_macros(self, domain):
        return await self._get_named_items(domain, Macro, "macro")

    async def get_domain_scenes(self, domain):
        return await self._get_named_items(domain, Scene, "scene")

    async def get_domain_device_servers(self, domain):
        return await self._get_named_items(
            domain, DeviceServer, "device_server")

    async def get_domain_device_instances(self, domain):
        return await self._get_named_items(
            domain, DeviceInstance, "device_instance")

    async def get_domain_device_configs(self, domain):
        return await self._get_named_items(
            domain, DeviceConfig, "device_config")

    async def get_subprojects_of_project(self, project: Project):
        links = await self._execute_all(
            select(ProjectSubproject).where(
                ProjectSubproject.project_id == project.id),
            order_by=ProjectSubproject.order)
        subprojects = []
        for link in links:
            sub = await self._execute_first(select(Project).where(
                Project.id == link.subproject_id))
            if sub:
                subprojects.append(sub)
        return subprojects

    async def get_scenes_of_project(self, project):
        return await self._execute_all(
            select(Scene).where(Scene.project_id == project.id),
            order_by=Scene.order)

    async def get_macros_of_project(self, project):
        return await self._execute_all(
            select(Macro).where(Macro.project_id == project.id),
            order_by=Macro.order)

    async def get_device_servers_of_project(self, project):
        return await self._execute_all(
            select(DeviceServer).where(DeviceServer.project_id == project.id),
            order_by=DeviceServer.order)

    async def get_device_instances_of_server(self, server):
        return await self._execute_all(
            select(DeviceInstance).where(
                DeviceInstance.device_server_id == server.id),
            order_by=DeviceInstance.order)

    async def get_device_configs_of_instance(self, instance):
        return await self._execute_all(
            select(DeviceConfig).where(
                DeviceConfig.device_instance_id == instance.id),
            order_by=DeviceConfig.order)

    async def get_project_from_uuid(self, uuid):
        project = await self._execute_first(
            select(Project).where(Project.uuid == uuid))
        return project

    async def get_project_from_id(self, id: int):
        project = await self._execute_first(
            select(Project).where(Project.id == id))
        set_project_timezone(project)
        return project

    async def get_scene_from_uuid(self, uuid):
        return await self._execute_first(
            select(Scene).where(Scene.uuid == uuid))

    async def get_macro_from_uuid(self, uuid):
        return await self._execute_first(
            select(Macro).where(Macro.uuid == uuid))

    async def get_device_server_from_uuid(self, uuid):
        return await self._execute_first(
            select(DeviceServer).where(DeviceServer.uuid == uuid))

    async def get_device_instance_from_uuid(self, uuid):
        return await self._execute_first(
            select(DeviceInstance).where(DeviceInstance.uuid == uuid))

    async def get_device_config_from_uuid(self, uuid):
        return await self._execute_first(
            select(DeviceConfig).where(DeviceConfig.uuid == uuid))

    async def get_domain_device_instances_by_name_part(
            self, domain: str, name_part: str):
        query = (
            select(DeviceInstance)
            .join(DeviceServer).join(Project).join(ProjectDomain)
            .where(ProjectDomain.name == domain)
            .filter(DeviceInstance.name.like(f'%{name_part}%'))
        )
        return await self._execute_all(query)

    async def get_domain_macro_instances_by_name_part(
            self, domain: str, name_part: str):
        query = (
            select(Macro)
            .join(Project).join(ProjectDomain)
            .where(ProjectDomain.name == domain)
            .filter(Macro.name.ilike(f'%{name_part}%')))
        return await self._execute_all(query)

    async def get_device_instance_project(self, instance: DeviceInstance):
        project = await self._execute_first(
            select(Project).join(DeviceServer).where(
                DeviceServer.id == instance.device_server_id)
        )
        set_project_timezone(project)
        return project
