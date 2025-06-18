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
import json
from zlib import adler32

from karabo.bound import BinarySerializerSchema
from karabo.bound.testing import ServerContext, eventLoop, sleepUntil


def test_adler():
    s = b'I love python, Hello world'
    t = adler32(s)
    assert t == 2073102698, "The adler32's behavior changed."


def test_schema(eventLoop: eventLoop):
    config = {"AdlerTestDevice": {"classId": "TestDevice",
                                  "middlelayerDevice": "Notthere"}}
    server = ServerContext(
        "testAdler",
        [f"init={json.dumps(config)}",
         "pluginNamespace=karabo.bound_device_test"])
    with server:
        remote = server.remote()
        sleepUntil(lambda: "AdlerTestDevice" in remote.getDevices(),
                   timeout=10)
        schema = remote.getDeviceSchema("AdlerTestDevice")
        ser = BinarySerializerSchema.create("Bin")
        schema = ser.save(schema)
        text = ("The generated schema changed. If this "
                "is desired, change the checksum in the code.")
        checksum = adler32(schema)
        assert checksum == 1915509074, text
