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

from sqlalchemy import insert
from sqlalchemy.ext.asyncio import AsyncSession, create_async_engine
from sqlalchemy.future import select
from sqlalchemy.orm import sessionmaker

from .models import Base, DeviceConfig
from .utils import ConfigurationDBError, datetime_from_string, utc_to_local


class DbHandle:
    def __init__(self, db_name: str):
        self.db_name = db_name
        self.engine = create_async_engine(
            f"sqlite+aiosqlite:///{self.db_name}")
        self.session_handle = sessionmaker(
            bind=self.engine, class_=AsyncSession,
            expire_on_commit=False)

    async def __aenter__(self):
        self.session = self.session_handle()
        return self.session

    async def __aexit__(self, exc_type, exc_value, traceback):
        await self.session.close()


class ConfigurationDatabase:

    def __init__(self, db_handle):
        self.db_handle = db_handle

    @property
    def path(self) -> Path:
        return Path(self.db_handle.db_name)

    @property
    def dirname(self) -> str | None:
        return self.path.parent if self.path.exists() else None

    async def assure_existing(self):
        """Ensures the database schema is created."""
        async with self.db_handle.engine.begin() as conn:
            await conn.run_sync(Base.metadata.create_all)

    async def delete(self):
        """Deletes the database schema and file."""
        async with self.db_handle.engine.begin() as conn:
            await conn.run_sync(Base.metadata.drop_all)
        await self.db_handle.engine.dispose()
        if self.path.exists():
            self.path.unlink()

    # Public Interface
    # --------------------------------------------------------------------

    async def list_configurations(
            self, device_id: str, name_part: str = "") -> dict:
        """Retrieves the set of device configurations related to a name part

        :param device_id: the id of the device
        :param name_part: the name part of the device configurations.
                          Empty means all named configurations.
        """
        async with self.db_handle as session:
            stmt = select(DeviceConfig.name, DeviceConfig.timestamp).where(
                DeviceConfig.device_id == device_id)
            if name_part:
                stmt = stmt.where(DeviceConfig.name.contains(name_part))
            result = await session.execute(stmt)
            return [
                {"name": name,
                 "timepoint": utc_to_local(timestamp)}
                for name, timestamp in result.fetchall()]

    def get_session(self):
        return self.db_handle

    async def list_devices(self) -> list[str]:
        """Retrieves the set of deviceIds

        :returns: list of deviceIds records (can be empty).
        """
        async with self.get_session() as session:
            stmt = select(DeviceConfig.device_id).distinct()
            result = await session.execute(stmt)
            devices = result.scalars().all()
            return [str(device) for device in devices]

    async def get_configuration(self, device_id: str, name: str) -> dict:
        """Retrieves a device configuration given its name.

        :param device_id: the id of the device.
        :param name: the name of the device configuration.

        :returns: dictionary result (can be empty). The configuration
                  timestamp is returned in ISO8601 format.
        """
        async with self.get_session() as session:
            stmt = select(DeviceConfig.timestamp,
                          DeviceConfig.config_data).where(
                DeviceConfig.device_id == device_id,
                DeviceConfig.name == name)
            result = await session.execute(stmt)
            config = result.fetchone()
            if config is None:
                return {}

        return {
            "name": name,
            "deviceId": device_id,
            "timestamp": utc_to_local(config[0]),
            "config": config[1]}

    async def save_configuration(self, name: str, configs: list[dict],
                                 timestamp: str | None = None):
        """Saves one or more device configurations.

        :param name: the configuration(s) name
        :param configs: a list of dictionaries with keys:
                        - "deviceId"
                        - "config" (Base64 encoded binary serialization of
                                    the configuration)
        :param timestamp: Optional ISO8601 timestamp;
                          if not provided, the current UTC timestamp is used.
        """
        if len(name) >= 80:
            raise ConfigurationDBError(
                "Please provide a name with less than 80 characters.")

        required_keys = ["deviceId", "config"]
        for index, config in enumerate(configs):
            if not all(key in config for key in required_keys):
                raise ConfigurationDBError(
                    f"Configuration #{index} is missing required keys.")

        async with self.get_session() as session:
            try:
                if timestamp is None:
                    timestamp = datetime.now(timezone.utc)
                else:
                    timestamp = datetime_from_string(timestamp)

                for config in configs:
                    stmt = insert(DeviceConfig).values(
                        device_id=config["deviceId"],
                        name=name,
                        timestamp=timestamp,
                        config_data=config["config"]
                    ).prefix_with("OR REPLACE")
                    await session.execute(stmt)

                await session.commit()
            except Exception as e:
                await session.rollback()
                raise ConfigurationDBError(f"{e}")
