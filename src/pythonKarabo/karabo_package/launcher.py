from asyncio import (coroutine, CancelledError, get_event_loop, set_event_loop,
                     create_subprocess_exec, wait_for, gather)
import sys
import os.path
from importlib import import_module
from subprocess import PIPE

if __name__ == "__main__":
    _, fn, cls, what = sys.argv
    if what in ("legacy", "schema"):
        sys.karabo_api = 1

from karabo.hash import BinaryWriter, BinaryParser
from karabo.eventloop import EventLoop


@coroutine
def newProcess(cls, config):
    def callback():
        loop = EventLoop()
        set_event_loop(loop)
        device = self.cls(self.config)
        device.run()
        try:
            loop.run_forever()
        finally:
            loop.close()
    try:
        yield from get_event_loop().run_in_executor(None, callback)
    except CancelledError:
        loop.stop()


@coroutine
def newThread(cls, config):
    def callback():
        loop = EventLoop()
        set_event_loop(loop)
        device = self.cls(self.config)
        device.run()
        try:
            loop.run_forever()
        finally:
            loop.close()
    try:
        yield from get_event_loop().run_in_executor(None, callback)
    except CancelledError:
        loop.stop()


@coroutine
def sameThread(cls, config):
    obj = cls(config)
    obj.run()
    while obj._tasks:
        try:
            yield from gather(*obj._tasks)
        except CancelledError:
            for t in obj._tasks:
                t.cancel()


@coroutine
def legacy(cls, config):
    child = yield from create_subprocess_exec(
        sys.executable, __file__, sys.modules[cls.__module__].__file__,
        cls.__classid__, "legacy", stdin=PIPE)
    BinaryWriter().writeToFile(config, child.stdin)
    child.stdin.close()
    try:
        yield from child.wait()
    except CancelledError:
        child.terminate()
        try:
            yield from wait_for(child.wait(), 3)
        except TimeoutError:
            child.kill()
            try:
                yield from wait_for(child.wait(), 3)
            except TimeoutError:
                print('unable to kill "{}"'.format(cls))


@coroutine
def getClassSchema_async(cls, rules=None):
    child = yield from create_subprocess_exec(
        sys.executable, __file__, sys.modules[cls.__module__].__file__,
        cls.__classid__, "schema", stdout=PIPE)
    stdout, stderr = yield from child.communicate()
    h = BinaryParser().read(stdout)
    return h["schema"]


if __name__ == "__main__":
    sys.path.insert(0, os.path.dirname(fn))
    assert fn.endswith(".py")
    import_module(os.path.basename(fn)[:-3])

    if what in ("legacy", "schema"):
        from karabo.device import PythonDevice
        from karabo.karathon import BinarySerializerHash
        from karabo.configurator import Configurator
        from karabo.karathon import Hash, Validator

    if what == "legacy":
        s = BinarySerializerHash.create("Bin")
        config = s.load(bytearray(sys.stdin.buffer.read()))
        schema = Configurator(PythonDevice).getSchema(cls)
        v = Validator()
        validated = v.validate(schema, config)
        device = Configurator(PythonDevice).create(cls, config)
        device.run()
    elif what == "schema":
        schema = Configurator(PythonDevice).getSchema(cls)
        h = Hash("schema", schema)
        s = BinarySerializerHash.create("Bin")
        sys.stdout.buffer.write(s.save(h))
    elif what == "run":
        from karabo.device import Device

        config = BinaryParser.read(sys.stdin.buffer)
        loop = EventLoop()
        Cls = Device.subclasses[cls]
        obj = Cls(config)
        obj.run()
        try:
            loop.run_forever()
        finally:
            loop.close()
