from contextlib import contextmanager

from karabo.middlelayer_api.tests.eventloop import async_tst, DeviceTest

from ..property_test import PropertyTestMDL



conf = {
    "classId": "PropertyTestMDL",
    "_deviceId_": "Test_PropertyTestMDL"
}


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.dev = PropertyTestMDL(conf)
        with cls.deviceManager(lead=cls.dev):
            yield

    @async_tst
    async def test_init(self):
        pass
