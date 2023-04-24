# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os
from contextlib import contextmanager

from karabo.middlelayer import Hash
from karabo.middlelayer_api.tests.eventloop import DeviceTest
from karabo.middlelayer_devices.project_manager import ProjectManager
from karabo.project_db.exist_db.database import ProjectDatabase
from karabo.project_db.exist_db.tests.util import stop_local_database
from karabo.project_db.exist_db.util import TESTDB_ADMIN_PASSWORD

from .projectdb_util import ConsumerDevice, VerificationProjectManager


class TestProjectManager(VerificationProjectManager, DeviceTest):
    _user = "admin"
    _password = TESTDB_ADMIN_PASSWORD

    def _db_init(self):
        return ProjectDatabase(self._user, self._password,
                               test_mode=True, init_db=True)

    def stop_local_database(self):
        stop_local_database()

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        host = os.getenv('KARABO_TEST_PROJECT_DB', 'localhost')
        port = int(os.getenv('KARABO_TEST_PROJECT_DB_PORT', '8181'))
        conf = Hash("_deviceId_", "projManTest")
        conf["projectDB"] = Hash("protocol", "exist_db",
                                 "exist_db", Hash("host", host,
                                                  "port", int(port)),
                                 "testMode", True)
        cls.local = ProjectManager(conf)
        cls.consume = ConsumerDevice({"_deviceId_": "consumeTest"})
        with cls.deviceManager(cls.local, cls.consume, lead=cls.local):
            yield
