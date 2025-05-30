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
from datetime import datetime, timezone
from pathlib import Path

from sqlalchemy import insert, select, update
from sqlalchemy.ext.asyncio import async_sessionmaker, create_async_engine
from sqlalchemy.orm import selectinload
from sqlmodel import SQLModel
from sqlmodel.ext.asyncio.session import AsyncSession

from .models import NamedDeviceConfig, NamedDeviceInstance
from .utils import ConfigurationDBError, datetime_from_string, utc_to_local


class ConfigurationDatabase:
    def __init__(self, db_name: str):
        self.db_name = db_name
        self.engine = create_async_engine(
            f"sqlite+aiosqlite:///{self.db_name}",
            echo=False,
            pool_pre_ping=True,
            connect_args={"check_same_thread": True})
        self.session = async_sessionmaker(
            bind=self.engine, class_=AsyncSession,
            expire_on_commit=False)

    @property
    def path(self) -> Path:
        return Path(self.db_name)

    @property
    def dirname(self) -> str | None:
        return str(self.path.parent) if self.path.exists() else None

    async def assure_existing(self):
        """Ensures the database schema is created."""
        async with self.engine.begin() as conn:
            await conn.run_sync(SQLModel.metadata.create_all)

    async def delete(self):
        """Deletes the database schema and file."""
        async with self.engine.begin() as conn:
            await conn.run_sync(SQLModel.metadata.drop_all)
        await self.engine.dispose()
        if self.path.exists():
            self.path.unlink()

    # Public Interface
    # --------------------------------------------------------------------

    async def list_configurations(
            self, device_id: str, name_part: str = "") -> list[dict]:
        """Retrieves the set of device configurations related to a name part.

        :param device_id: The ID of the device.
        :param name_part: Partial name filter for configurations.
                        If empty, retrieves all configurations.
        :return: List of configuration dictionaries (can be empty).
        """
        async with self.session() as session:
            stmt = (
                select(NamedDeviceInstance)
                .where(NamedDeviceInstance.device_id == device_id)
                # Orm query
                .options(selectinload(NamedDeviceInstance.configurations)))

            result = await session.exec(stmt)
            device = result.scalars().first()
            if not device:
                return []

            configurations = device.configurations
            if name_part:
                configurations = [config for config in configurations
                                  if name_part in config.name]

            return [
                {"name": config.name,
                 "timepoint": utc_to_local(config.timestamp),
                 "last_loaded": utc_to_local(config.last_loaded)}
                for config in configurations]

    def session(self):
        return self.db_handle

    async def delete_configuration(self, device_id: str, name: str):
        """Delete a device configuration by `name` and `deviceId`"""
        async with self.session() as session:
            stmt = (
                select(NamedDeviceConfig)
                .where(
                    NamedDeviceConfig.device_id == device_id,
                    NamedDeviceConfig.name == name)
            )
            result = await session.exec(stmt)
            config = result.scalars().first()
            if not config:
                raise ConfigurationDBError(
                    f"Configuration for {device_id} - {name} not available.")

            try:
                await session.delete(config)
                # XXX: Check if the parent device has any remaining
                # configurations
                stmt = (
                    select(NamedDeviceInstance)
                    .where(NamedDeviceInstance.device_id == device_id)
                    .options(selectinload(NamedDeviceInstance.configurations))
                )
                result = await session.exec(stmt)
                device = result.scalars().first()
                if device and not device.configurations:
                    await session.delete(device)
                await session.commit()
            except Exception as e:
                await session.rollback()
                raise ConfigurationDBError(
                    f"Failed to delete configuration: {e}")

    async def list_devices(self) -> list[str]:
        """Retrieves the set of deviceIds

        :returns: list of deviceIds records (can be empty).
        """
        async with self.session() as session:
            stmt = select(NamedDeviceInstance.device_id).distinct()
            result = await session.exec(stmt)
            return result.scalars().all()

    async def get_configuration(self, device_id: str, name: str) -> dict:
        """Retrieve a device configuration with name and update `last_loaded`

        :param device_id: The ID of the device.
        :param name: The name of the configuration.
        :return: Configuration dictionary (or None if not found).
        """
        async with self.session() as session:
            stmt = (
                select(NamedDeviceConfig)
                .where(
                    NamedDeviceConfig.device_id == device_id,
                    NamedDeviceConfig.name == name
                )
                # Lock for update
                .with_for_update()
            )
            result = await session.exec(stmt)
            config = result.scalars().first()
            if not config:
                return {}

            update_stmt = (
                update(NamedDeviceConfig)
                .where(NamedDeviceConfig.id == config.id)
                .values(last_loaded=datetime.now(timezone.utc))
            )

            await session.exec(update_stmt)
            await session.commit()

            return {
                "name": name,
                "deviceId": device_id,
                "timestamp": utc_to_local(config.timestamp),
                "config": config.config_data}

    async def save_configuration(
            self, name: str, configs: list[dict],
            timestamp: str | None = None):
        """Saves one or more device configurations, ensuring the Device exists.

        :param name: The configuration(s) name.
        :param configs: A list of dictionaries with keys:
                        - "deviceId"
                        - "config"
        :param timestamp: Optional ISO8601 timestamp;
                          defaults to current UTC time.
        """
        if len(name) >= 80:
            text = "Please provide a name with less than 80 characters."
            raise ConfigurationDBError(text)

        required_keys = ["deviceId", "config"]
        for index, config in enumerate(configs):
            if not all(key in config for key in required_keys):
                raise ConfigurationDBError(
                    f"Configuration #{index} is missing required keys.")

        async with self.session() as session:
            try:
                timestamp = (datetime_from_string(timestamp) if timestamp
                             is not None else datetime.now(timezone.utc))

                for config in configs:
                    device_id = config["deviceId"]
                    stmt_device = select(NamedDeviceInstance).where(
                        NamedDeviceInstance.device_id == device_id)
                    device = (await session.exec(
                        stmt_device)).scalars().first()
                    if not device:
                        session.add(NamedDeviceInstance(device_id=device_id))

                    stmt_config = (
                        insert(NamedDeviceConfig)
                        .values(
                            device_id=device_id,
                            name=name,
                            timestamp=timestamp,
                            config_data=config["config"]
                        ).prefix_with("OR REPLACE"))
                    await session.exec(stmt_config)
                await session.commit()
            except Exception as e:
                await session.rollback()
                raise ConfigurationDBError(f"Unexpected Error: {e}")
