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

# db_engine and session_gen (Session Factory) instances and initializers

import os

from sqlalchemy.ext.asyncio import (
    AsyncEngine, async_sessionmaker, create_async_engine)
from sqlalchemy.pool import StaticPool
from sqlmodel.ext.asyncio.session import AsyncSession

DB_POOL_RECYCLE_SECS: int = int(os.environ.get(
        "KARABO_PROJECT_DB_POOL_RECYCLE_SECS", 100))


def init_async_db_engine(
    user: str, password: str, server: str,
    port: int, db_name: str) -> tuple[
        AsyncEngine, async_sessionmaker[AsyncSession]]:
    db_url = f"mysql+aiomysql://{user}:{password}@{server}:{port}/{db_name}"

    db_engine = create_async_engine(
        db_url,
        echo=False,
        pool_recycle=DB_POOL_RECYCLE_SECS,
        pool_pre_ping=True,
        pool_size=20,
        max_overflow=10,
    )

    session_gen = async_sessionmaker(
        bind=db_engine,
        expire_on_commit=False,
        class_=AsyncSession,
        autoflush=False,
        autocommit=False,
    )

    return db_engine, session_gen


def init_test_async_db_engine() -> tuple[
        AsyncEngine, async_sessionmaker[AsyncSession]]:
    db_engine = create_async_engine(
        "sqlite+aiosqlite:///:memory:",
        echo=False,
        connect_args={"check_same_thread": False},
        poolclass=StaticPool)

    session_gen = async_sessionmaker(
        bind=db_engine,
        expire_on_commit=False,
        class_=AsyncSession,
        autoflush=False,
        autocommit=False)

    return db_engine, session_gen
