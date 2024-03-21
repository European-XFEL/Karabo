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
        cmdLine = ['foo', 'serverId=goo', 'autoStart[0]={a.b=c', 'x={y=12',
                   'a.c=1}}']
        res, parsed = self.runner.parseCommandLine(cmdLine)
        self.assertTrue(res)
        self.assertTrue(parsed.has("serverId"))
        self.assertEqual(parsed.get("serverId"), "goo")
        self.assertTrue(parsed.has("autoStart"))
        autoStart = parsed.get("autoStart")[0]
        self.assertTrue(autoStart.has("a.b"))
        self.assertTrue(autoStart.has("x.y"))
        self.assertTrue(autoStart.has("x.a.c"))
        self.assertEqual(autoStart.get("a.b"), "c")
        self.assertEqual(autoStart.get("x.y"), "12")
        self.assertEqual(autoStart.get("x.a.c"), "1")

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
        self.assertEqual(parsed.get("autoStart[0].TheClassName.deviceId"),
                         "deviceId1")
        self.assertEqual(parsed.get(
            "autoStart[0].TheClassName.stringProperty"), "")
        self.assertEqual(parsed.get(
            "autoStart[0].TheClassName.floatProperty"), 42.1)
        self.assertEqual(parsed.get(
            "autoStart[0].TheClassName.node.stringProperty"), "Value1")
        self.assertEqual(parsed.get(
            "autoStart[1].TheClassName.deviceId"), "deviceId2")
        self.assertEqual(parsed.get(
            "autoStart[1].TheClassName.stringProperty"), "1.2.3:14")
        self.assertEqual(parsed.get(
            "autoStart[1].TheClassName.node.stringProperty"), "Value2")

    def test_argument_parser_both_autostart_init(self):
        init_string = """
init={
    "deviceId1": {
        "classId": "TheClassName",
        "stringProperty": "Value",
        "floatProperty": 42,
        "node": {
            "stringProperty": "Value"
        }
    },
    "deviceId2": {
        "classId": "TheClassName",
        "stringProperty": "Value",
        "floatProperty": 42,
        "node": {
            "stringProperty": "Value"
        }
    }
}
                  """
        cmdLine = ['foo', 'serverId=bingo', init_string,
                   'autostart[0]='
                   '{DataLoggerManager.serverList=dls1,dls2,dls3,dls4']
        with self.assertRaises(SyntaxError):
            res, parsed = self.runner.parseCommandLine(cmdLine)

    def test_argument_parser_failure1(self):
        with self.assertRaises(SyntaxError):
            res, parsed = self.runner.parseCommandLine(
                ['foo', 'serverId=bar', 'autoStart[0]={a.b=c', 'x={y=12',
                 'a.c=1}'])

    def test_argument_parser_failure2(self):
        with self.assertRaises(SyntaxError):
            res, parsed = self.runner.parseCommandLine(
                ['foo', 'serverId=bar', 'autoStart[0]={a.b=c', 'x={y=12',
                 'a.c=1}}}'])

    def test_argument_parser_failure3(self):
        with self.assertRaises(SyntaxError):
            res, parsed = self.runner.parseCommandLine(
                ['foo', 'serverId=bar', 'autoStart[0]=}a.b=c', 'x={y=12',
                 'a.c=1}}'])

    def test_argument_parser_failure4(self):
        with self.assertRaises(SyntaxError):
            res, parsed = self.runner.parseCommandLine(
                ['foo', 'serverId=bar', 'autoStart[0]={{a.b=c}', 'x={y=12',
                 'a.c=1}}'])
