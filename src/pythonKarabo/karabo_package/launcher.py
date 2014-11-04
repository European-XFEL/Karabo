import sys
import os.path
from importlib import import_module

_, fn, cls, version, what = sys.argv

sys.path.insert(0, os.path.dirname(fn))
sys.karabo_api = int(version)

assert fn.endswith(".py")

import_module(os.path.basename(fn)[:-3])

from karabo.device import PythonDevice
from karabo.karathon import BinarySerializerHash
from karabo.configurator import Configurator
from karabo.karathon import Hash, Validator

if what == "run":
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
