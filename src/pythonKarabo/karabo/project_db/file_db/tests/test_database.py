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
import contextlib
import os
import tempfile
from unittest import TestCase

from karabo.project_db.tests.base import ProjectDatabaseVerification

from ..database import ProjectDatabase


class TestFileProjectDatabase(ProjectDatabaseVerification, TestCase):
    @classmethod
    def setUpClass(cls):
        base_dir = os.path.join(os.environ['KARABO'], 'var', 'data')
        with contextlib.ExitStack() as stack:
            cls.test_dir = tempfile.TemporaryDirectory(dir=base_dir)
            stack.enter_context(cls.test_dir)

    def _db_init(self):
        return ProjectDatabase(
            str(self.test_dir), test_mode=True, init_db=True)
