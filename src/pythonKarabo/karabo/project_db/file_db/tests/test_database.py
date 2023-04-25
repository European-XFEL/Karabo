# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
