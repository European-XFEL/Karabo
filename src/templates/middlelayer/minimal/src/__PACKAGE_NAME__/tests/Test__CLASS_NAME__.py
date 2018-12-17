#############################################################################
# Author: __EMAIL__
# Created on __DATE__
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from contextlib import contextmanager
import unittest

from karabo.middlelayer_api.tests.eventloop import async_tst, DeviceTest

from ..__CLASS_NAME__ import __CLASS_NAME__


conf = {
    "classId": "__CLASS_NAME__",
    "_deviceId_": "Test__CLASS_NAME__",
    "greeting": "buongiorno"
}


class Test__CLASS_NAME__(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.dev = __CLASS_NAME__(conf)
        with cls.deviceManager(lead=cls.dev):
            yield

    @async_tst
    def test_greet(self):
        for greet in ("Buongiorno", "Guten Tag", "Moin Moin"):
            self.dev.greeting = greet
            self.assertEqual(self.dev.greeting.value, greet)
            yield from self.dev.hello()
            self.assertEqual(self.dev.greeting.value, "Hello world!")
