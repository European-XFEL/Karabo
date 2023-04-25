# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
