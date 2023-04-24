# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os
import tempfile
from contextlib import contextmanager

from karabo.middlelayer_api.tests.eventloop import DeviceTest
from karabo.middlelayer_devices.project_manager import ProjectManager
from karabo.native import Hash
from karabo.project_db.file_db.database import ProjectDatabase

from .projectdb_util import ConsumerDevice, VerificationProjectManager


class TestFileProjectManager(VerificationProjectManager, DeviceTest):
    def _db_init(self):
        return ProjectDatabase(self.dir, test_mode=True, init_db=True)

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        base_dir = os.path.join(os.environ['KARABO'], 'var', 'data')
        with tempfile.TemporaryDirectory(dir=base_dir) as cls.dir:
            path = str(os.path.relpath(cls.dir, base_dir))
            conf = Hash("_deviceId_", "projManTest")
            conf["projectDB"] = Hash("protocol", "file_db",
                                     "file_db", Hash("dir", path),
                                     "testMode", True)
            cls.local = ProjectManager(conf)
            cls.consume = ConsumerDevice({"_deviceId_": "consumeTest"})
            with cls.deviceManager(cls.local, cls.consume, lead=cls.local):
                yield
