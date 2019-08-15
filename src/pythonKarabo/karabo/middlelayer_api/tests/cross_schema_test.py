from asyncio import create_subprocess_exec
import os
import os.path
from subprocess import PIPE
import sys
from unittest import main
from zlib import adler32

from .eventloop import async_tst, DeviceTest


class Tests(DeviceTest):
    def setUp(self):
        self.__starting_dir = os.curdir
        os.chdir(os.path.dirname(os.path.abspath(__file__)))
        self.process = None

    def tearDown(self):
        if self.process is not None and self.process.returncode is None:
            self.process.kill()
            self.loop.run_until_complete(self.process.wait())
            self.process = None
        os.chdir(self.__starting_dir)

    @async_tst
    def test_schema(self):
        self.process = yield from create_subprocess_exec(
            sys.executable, "-m", "karabo.bound_api.launcher",
            "schema", "karabo.bound_device_test", "TestDevice",
            stdout=PIPE)
        schema = yield from self.process.stdout.read()
        yield from self.process.wait()
        self.assertEqual(adler32(schema), 287752350,
                         "The generated schema changed. If this is "
                         "desired, change the checksum in the code.")


if __name__ == "__main__":
    main()
