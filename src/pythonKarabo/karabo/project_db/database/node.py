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

import os
from pathlib import Path

from karabo.native import AccessMode, Bool, Configurable, String, UInt32

from .sql_database import SQLDatabase


def get_db_credentials():
    user = os.getenv('KARABO_PROJECT_DB_USER', None)
    password = os.getenv('KARABO_PROJECT_DB_PASSWORD', None)
    return user, password


class RemoteNode(Configurable):
    host = String(
        defaultValue=os.getenv("KARABO_PROJECT_DB_HOST", "localhost"),
        displayedName="Database host")

    port = UInt32(
        displayedName="Port",
        defaultValue=int(os.getenv("KARABO_PROJECT_DB_PORT", "3306")))

    dbName = String(
        defaultValue=os.getenv("KARABO_PROJECT_DB_DBNAME", "projectDB"),
        displayedName="Database name")

    removeOrphans = Bool(
        displayedName="Remove Orphan Records",
        description=(
            "Should orphan records be removed on project save (True) or "
            "later by a cleaning batch job?"),
        defaultValue=False,
        accessMode=AccessMode.READONLY)

    async def get_db(self, test_mode=False, init_db=False):
        user, password = get_db_credentials()
        db = SQLDatabase(
            user, password, server=self.host.value, port=self.port.value,
            db_name=self.dbName.value, remove_orphans=self.removeOrphans.value)
        await db.initialize()
        return db


class LocalNode(Configurable):

    dbName = String(
        defaultValue=os.getenv("KARABO_PROJECT_DB_DBNAME", "projectDB.db"),
        displayedName="Database name",
        description="The filename placed in var/project_db/")

    removeOrphans = Bool(
        displayedName="Remove Orphan Records",
        description=(
            "Should orphan records be removed on project save (True) or "
            "later by a cleaning batch job?"),
        defaultValue=False,
        accessMode=AccessMode.READONLY)

    async def get_db(self, test_mode=False, init_db=False):
        folder = Path(os.environ["KARABO"]).joinpath(
            "var", "data", "project_db")
        path = folder / self.dbName.value
        path.parent.mkdir(parents=True, exist_ok=True)
        db = SQLDatabase(db_name=path, local=True,
                         remove_orphans=self.removeOrphans.value)
        await db.initialize()
        return db
