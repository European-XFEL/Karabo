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

from ..runner import Runner


class Schema_TestCase(unittest.TestCase):
    def setUp(self):
        self.runner = Runner('someServer')

    def test_argument_parser(self):
        cmdLine = ['foo', 'serverId=goo',
                   'init={"a.b": "c","x": {"y": 12, "a.c": 1}}']
        res, parsed = self.runner.parseCommandLine(cmdLine)
        self.assertTrue(res)
        self.assertTrue(parsed.has("serverId"))
        self.assertEqual(parsed.get("serverId"), "goo")
        # 'parsed' is a Hash with 'init'...
        self.assertTrue(parsed.has("init"))
        initjson = parsed.get("init")
        self.assertEqual(initjson, '{"a.b": "c","x": {"y": 12, "a.c": 1}}')

    def test_argument_parser_success2(self):
        cmdLine = ['foo', 'serverId=bingo', 'a[0].b=1', 'a[0].c=2',
                   'a[0].e={x=15', 'y=88}']
        res, parsed = self.runner.parseCommandLine(cmdLine)
        self.assertTrue(res)
        self.assertTrue(parsed.has("serverId"))
        self.assertEqual(parsed.get("serverId"), "bingo")
        self.assertTrue(parsed.has("a[0].b"))
        self.assertTrue(parsed.has("a[0].c"))
        self.assertTrue(parsed.has("a[0].e"))
        self.assertEqual(parsed.get("a[0].b"), "1")
        self.assertEqual(parsed.get("a[0].c"), "2")
        self.assertEqual(parsed.get("a[0].e.x"), "15")
        self.assertEqual(parsed.get("a[0].e.y"), "88")

    def test_argument_parser_initString(self):
        init_string = """
init={
    "deviceId1": {
        "classId": "TheClassName",
        "stringProperty": "",
        "floatProperty": 42.1,
        "node": {
            "stringProperty": "Value1"
        }
    },
    "deviceId2": {
        "classId": "TheClassName",
        "stringProperty": "1.2.3:14",
        "floatProperty": 42,
        "node": {
            "stringProperty": "Value2"
        }
    }
}
                  """
        cmdLine = ['foo', 'serverId=bingo', init_string]

        res, parsed = self.runner.parseCommandLine(cmdLine)
        self.assertTrue(res)

    def test_argument_parser_autostart(self):
        # Test that old syntax leads to bail out
        cmdLine = ['foo', 'serverId=bingo',
                   'autostart[0]='
                   '{DataLoggerManager.serverList=dls1,dls2,dls3,dls4']
        with self.assertRaises(SyntaxError):
            res, parsed = self.runner.parseCommandLine(cmdLine)
