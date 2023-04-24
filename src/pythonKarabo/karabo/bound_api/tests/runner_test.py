# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
