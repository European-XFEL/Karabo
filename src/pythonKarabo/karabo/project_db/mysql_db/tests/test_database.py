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

import unittest

from karabo.project_db.tests.base import ProjectDatabaseVerification

from ..database import ProjectDatabase


# TestCase derivation is declared to keep compatibility with test cases
# inherited from ProjectDatabaseVerification. To be removed when all the other
# back-end tests are migrated from unittest to pytest
class TestProjectDatabase(ProjectDatabaseVerification,
                          unittest.TestCase):

    def _db_init(self):
        # In test_mode an in-memory SQLite database is used. No need for
        # database connection parameters (e.g. user, db_name).
        return ProjectDatabase(test_mode=True)
