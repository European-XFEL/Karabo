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

from karabo.native import AccessLevel, Configurable, String, UInt32

from .sql_database import SQLDatabase


def get_db_credentials():
    user = os.getenv('KARABO_PROJECT_DB_USER', None)
    password = os.getenv('KARABO_PROJECT_DB_PASSWORD', None)
    return user, password


class MySqlNode(Configurable):
    host = String(
        defaultValue=os.getenv("KARABO_PROJECT_DB_HOST", "localhost"),
        displayedName="Database host",
        requiredAccessLevel=AccessLevel.EXPERT)

    port = UInt32(
        displayedName="Port",
        defaultValue=int(os.getenv("KARABO_PROJECT_DB_PORT", "3306")),
        requiredAccessLevel=AccessLevel.EXPERT)

    dbName = String(
        defaultValue=os.getenv("KARABO_PROJECT_DB_DBNAME", "projectDB"),
        displayedName="Database name",
        requiredAccessLevel=AccessLevel.EXPERT)

    def get_db(self, test_mode=False, init_db=False):
        user, password = get_db_credentials()
        return SQLDatabase(
            user, password, server=self.host.value, port=self.port.value,
            db_name=self.dbName.value)
