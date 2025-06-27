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

import getpass
import os
from pathlib import Path

from karabo.native import (
    AccessMode, Configurable, String, UInt32, VectorRegexString, isSet)

from .sql_database import SQLDatabase


def get_db_credentials():
    user = os.getenv('KARABO_PROJECT_DB_USER', None)
    password = os.getenv('KARABO_PROJECT_DB_PASSWORD', None)
    return user, password


def topicDomain() -> str:
    topic = getpass.getuser()
    if "KARABO_BROKER_TOPIC" in os.environ:
        topic = os.environ["KARABO_BROKER_TOPIC"]
    return topic


_TOPIC_DOMAIN = topicDomain()


class RemoteNode(Configurable):
    host = String(
        displayedName="Database host",
        accessMode=AccessMode.INITONLY)

    port = UInt32(
        displayedName="Port",
        accessMode=AccessMode.INITONLY)

    dbName = String(
        displayedName="Database name",
        accessMode=AccessMode.INITONLY)

    async def get_db(self, test_mode=False, init_db=False):
        if not isSet(self.host):
            self.host = os.getenv("KARABO_PROJECT_DB_HOST", "localhost"),
        if not isSet(self.port):
            self.port = int(os.getenv("KARABO_PROJECT_DB_PORT", "3306"))
        if not isSet(self.dbName):
            self.dbName = os.getenv("KARABO_PROJECT_DB_DBNAME", "projectDB.db")

        user, password = get_db_credentials()
        db = SQLDatabase(
            user, password, server=self.host.value, port=self.port.value,
            db_name=self.dbName.value)
        await db.initialize()
        return db


class LocalNode(Configurable):

    dbName = String(
        displayedName="Database name",
        description="The filename placed in var/project_db/",
        accessMode=AccessMode.INITONLY)

    requiredDomains = VectorRegexString(
        displayedName="Required Domains",
        regex=r'^\w+$',
        description=(
            "The ProjectManager ensures that domains in this list are in the "
            "database when it starts."),
        accessMode=AccessMode.INITONLY)

    async def get_db(self, test_mode=False, init_db=False):
        if not isSet(self.dbName):
            self.dbName = os.getenv("KARABO_PROJECT_DB_DBNAME", "projectDB.db")

        folder = Path(os.environ["KARABO"]).joinpath(
            "var", "data", "project_db")
        path = folder / self.dbName.value
        path.parent.mkdir(parents=True, exist_ok=True)
        db = SQLDatabase(db_name=path, local=True)
        if isSet(self.requiredDomains):
            required_domains = self.requiredDomains
        else:
            required_domains = [_TOPIC_DOMAIN]

        await db.initialize()
        await db.assure_domains(required_domains)
        return db
