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

from sqlalchemy.engine import Engine
from sqlalchemy.orm import sessionmaker
from sqlmodel import create_engine
from sqlmodel.orm.session import Session

DB_POOL_RECYCLE_SECS: int = int(os.environ.get(
        "KARABO_PROJECT_DB_POOL_RECYCLE_SECS", 100))


def init_db_engine(user: str, password: str, server: str,
                   port: int, db_name: str) -> tuple[Engine, sessionmaker]:
    db_engine = create_engine(
            f"mysql+pymysql://{user}:{password}@{server}:{port}/{db_name}",
            # Being explicit in case SQL debugging is needed - echo False is
            # already the default.
            echo=False,
            pool_recycle=DB_POOL_RECYCLE_SECS,
            pool_pre_ping=True)

    session_gen = sessionmaker(
        autocommit=False,
        autoflush=False,
        bind=db_engine,
        class_=Session,
        expire_on_commit=False,
    )
    return (db_engine, session_gen)
