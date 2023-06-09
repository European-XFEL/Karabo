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
from unittest import TestCase

from karabo.project_db.tests.base import ProjectDatabaseVerification

from ..database import ProjectDatabase
from ..util import TESTDB_ADMIN_PASSWORD
from .util import stop_local_database


class TestProjectDatabase(ProjectDatabaseVerification, TestCase):
    user = "admin"
    password = TESTDB_ADMIN_PASSWORD

    def stop_local_database(self):
        stop_local_database()

    def _db_init(self):
        return ProjectDatabase(self.user, self.password,
                               test_mode=True, init_db=True)
