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

from sqlalchemy import Column, Text, UniqueConstraint
from sqlmodel import Field, Relationship, SQLModel


class ProjectDomain(SQLModel, table=True):
    __tablename__ = "ProjectDomain"

    id: int | None = Field(default=None, primary_key=True)
    name: str = Field(max_length=64, unique=True, nullable=False)
    projects: list["Project"] = Relationship(back_populates="project_domain")


class DatabaseMetadata(SQLModel, table=True):
    __tablename__ = "DatabaseMetadata"

    id: int | None = Field(default=None, primary_key=True)
    schema_version: int = Field(nullable=False, unique=True)


class Project(SQLModel, table=True):
    __tablename__ = "Project"

    id: int | None = Field(default=None, primary_key=True)
    # UUIDs are a legacy from ExistDB. They are being kept and are required
    # for backwards compatibility with ExistDB, and to allow partial
    # migrations from ExistDB (that will be needed if for some reason both
    # storages must coexist for some time; e.g. some topics on ExistDB and some
    # on a Relational Database).
    uuid: str = Field(max_length=64, nullable=False, unique=True)
    name: str = Field(max_length=128, nullable=False)
    is_trashed: bool = Field(default=False)

    date: datetime.datetime | None = Field(default=None, nullable=True)

    last_loaded: datetime.datetime | None = Field(default=None, nullable=True)

    project_domain_id: int = Field(foreign_key="ProjectDomain.id",
                                   nullable=False)
    project_domain: ProjectDomain = Relationship(back_populates="projects")

    device_servers: list["DeviceServer"] = Relationship(
        back_populates="project")

    scenes: list["Scene"] = Relationship(back_populates="project")

    macros: list["Macro"] = Relationship(back_populates="project")

    def __repr__(self):
        return self.__str__()

    def __str__(self):
        return (
            f"{self.project_domain.name}/{self.name} ({self.uuid} - "
            f"{self.is_trashed})")


class ProjectSubproject(SQLModel, table=True):
    """The many-to-many relationship between projects and subprojects."""
    __tablename__ = "ProjectSubproject"

    id: int | None = Field(default=None, primary_key=True)
    project_id: int = Field(foreign_key="Project.id", nullable=False)
    subproject_id: int = Field(foreign_key="Project.id", nullable=False)

    # Order of subprojects of a project
    order: int = Field(default=0, nullable=False)

    __table_args__ = (UniqueConstraint("project_id", "subproject_id"), )


class DeviceServer(SQLModel, table=True):
    """A Device Server "captured" from a live topology."""

    __tablename__ = "DeviceServer"

    id: int | None = Field(default=None, primary_key=True)
    name: str = Field(max_length=128)
    uuid: str = Field(max_length=64, nullable=False, unique=True)

    date: datetime.datetime | None = Field(default=None, nullable=True)

    project_id: int | None = Field(foreign_key="Project.id", nullable=True)
    project: Project = Relationship(back_populates="device_servers")

    device_instances: list["DeviceInstance"] = Relationship(
        back_populates="device_server")

    # Order of the device servers in a project
    order: int = Field(default=0, nullable=False)


class DeviceInstance(SQLModel, table=True):
    __tablename__ = "DeviceInstance"

    id: int | None = Field(default=None, primary_key=True)
    name: str = Field(max_length=256, nullable=False, index=True)
    uuid: str = Field(max_length=64, nullable=False, unique=True)
    # NOTE: if a device instance changes its class_id, all the stored
    #       configurations must be erased upon an user agreement (on normal
    #       Karabo use). For the migration script, an update that changes the
    #       class_id of a device instance that already has some configuration
    #       stored triggers a name mangling and adds a record to the proper
    #       migration transformation registry table.
    class_id: str = Field(max_length=64, nullable=False, index=True)

    device_server_id: int = Field(foreign_key="DeviceServer.id",
                                  nullable=True)

    date: datetime.datetime | None = Field(default=None, nullable=True)

    device_server: DeviceServer = Relationship(
        back_populates="device_instances")
    configs: list["DeviceConfig"] = Relationship(back_populates="device")

    # The order of the devices instances of a device server
    order: int = Field(default=0, nullable=False)


class DeviceConfig(SQLModel, table=True):
    __tablename__ = "DeviceConfig"

    id: int | None = Field(default=None, primary_key=True)
    uuid: str = Field(max_length=64, nullable=False, unique=True)
    name: str = Field(max_length=128, nullable=False)
    # Max size for configuration data is 4Mb
    config_data: str = Field(sa_column=Column(Text(4_194_304), nullable=False))

    date: datetime.datetime | None = Field(default=None, nullable=True)

    device_instance_id: int = Field(foreign_key="DeviceInstance.id",
                                    nullable=True)
    device: DeviceInstance = Relationship(back_populates="configs")

    # The order of the devices configs of a device instance
    order: int = Field(default=0, nullable=False)

    # Is this the active config for among its device instance configs?
    is_active: bool = Field(default=False)

    __table_args__ = (UniqueConstraint("device_instance_id", "name"),)


class Scene(SQLModel, table=True):
    __tablename__ = "Scene"

    id: int | None = Field(default=None, primary_key=True)
    uuid: str = Field(max_length=64, nullable=False, unique=True)
    name: str = Field(max_length=128, nullable=False)
    # Max size for svg_data is 4Mb
    svg_data: str = Field(sa_column=Column(Text(4_194_304), nullable=False))

    date: datetime.datetime | None = Field(default=None, nullable=True)

    project_id: int | None = Field(foreign_key="Project.id", nullable=True)
    project: Project = Relationship(back_populates="scenes")

    # The order of the scenes in a project
    order: int = Field(default=0, nullable=False)


class Macro(SQLModel, table=True):
    __tablename__ = "Macro"

    id: int | None = Field(default=None, primary_key=True)
    uuid: str = Field(max_length=64, nullable=False, unique=True)
    name: str = Field(max_length=128, nullable=False)
    # Max size for macro body is 4Mb
    body: str = Field(sa_column=Column(Text(4_194_304), nullable=False))

    date: datetime.datetime | None = Field(default=None, nullable=True)

    project_id: int | None = Field(foreign_key="Project.id", nullable=True)
    project: Project = Relationship(back_populates="macros")

    # The order of the macros in a project
    order: int = Field(default=0, nullable=False)


class SceneLinkedScene(SQLModel, table=True):
    """The many-to-many relationship between scenes and linked scenes."""
    __tablename__ = "SceneLinkedScene"

    id: int | None = Field(default=None, primary_key=True)
    scene_id: int = Field(foreign_key="Scene.id", nullable=False)
    linked_scene_id: int = Field(foreign_key="Scene.id", nullable=False)

    __table_args__ = (UniqueConstraint("scene_id", "linked_scene_id"), )
