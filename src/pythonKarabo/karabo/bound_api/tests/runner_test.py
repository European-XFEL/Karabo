
import unittest

from ..runner import Runner


class  Schema_TestCase(unittest.TestCase):

    def test_argument_parser(self):
        runner = Runner('someServer')
        cmdLine = ['foo', 'serverId=goo', 'autoStart[0]={a.b=c', 'a.c=1}']
        res, parsed = runner.parseCommandLine(cmdLine)
        self.assertTrue(res)
        self.assertTrue(parsed.has("serverId"))
        self.assertEqual(parsed.get("serverId"), "goo")
        self.assertTrue(parsed.has("autoStart"))
        autoStart = parsed.get("autoStart")[0]
        self.assertTrue(autoStart.has("a.b"))
        self.assertTrue(autoStart.has("a.c"))
        self.assertEqual(autoStart.get("a.b"), "c")
        self.assertEqual(autoStart.get("a.c"), "1")