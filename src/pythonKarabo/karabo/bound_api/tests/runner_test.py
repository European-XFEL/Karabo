
import unittest

from ..runner import Runner


class  Schema_TestCase(unittest.TestCase):
    def setUp(self):
        self.runner = Runner('someServer')

    def test_argument_parser(self):
        runner = Runner('someServer')
        cmdLine = ['foo', 'serverId=goo', 'autoStart[0]={a.b=c', 'x={y=12', 'a.c=1}}']
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

    def test_argument_parser_failure1(self):
        with self.assertRaises(SyntaxError):
            res, parsed = self.runner.parseCommandLine(
                ['foo', 'serverId=bar', 'autoStart[0]={a.b=c', 'x={y=12', 'a.c=1}'])

    def test_argument_parser_failure2(self):
        with self.assertRaises(SyntaxError):
            res, parsed = self.runner.parseCommandLine(
                ['foo', 'serverId=bar', 'autoStart[0]={a.b=c', 'x={y=12', 'a.c=1}}}'])

    def test_argument_parser_failure3(self):
        with self.assertRaises(SyntaxError):
            res, parsed = self.runner.parseCommandLine(
                ['foo', 'serverId=bar', 'autoStart[0]=}a.b=c', 'x={y=12', 'a.c=1}}'])
